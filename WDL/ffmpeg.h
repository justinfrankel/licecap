/*
    WDL - ffmpeg.h
    Copyright (C) 2005 Cockos Incorporated
    Copyright (C) 1999-2004 Nullsoft, Inc. 
  
    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
      
*/

#ifndef _WDL_FFMPEG_H
#define _WDL_FFMPEG_H

#ifdef _MSC_VER
#include "../sdks/ffmpeg/include/stdint.h"
#include "../sdks/ffmpeg/include/inttypes.h"
#endif

extern "C"
{
#include "../sdks/ffmpeg/include/libavformat/avformat.h"
#include "../sdks/ffmpeg/include/libswscale/swscale.h"
};

#include "queue.h"

#ifndef INT64_C
#define INT64_C(val) val##i64
#endif

class WDL_VideoEncode
{
public:
  //bitrates are in kbps
  WDL_VideoEncode(const char *format, int width, int height, double fps, int bitrate, const char *audioformat=NULL, int asr=44100, int ach=2, int abitrate=0)
  {
    m_init = 0;
    m_img_resample_ctx = NULL;
    m_stream = NULL;
    m_astream = NULL;
    m_video_enc = NULL;
    m_audio_enc = NULL;
    m_bit_buffer = NULL;
    avcodec_get_frame_defaults(&m_cvtpic);
    
    //initialize FFMpeg
    {
      static int init = 0;
      if(!init) av_register_all();
      init = 1;
    }

    m_ctx = av_alloc_format_context();
    AVOutputFormat *fmt = guess_format(format, NULL, NULL);
    if(!m_ctx || !fmt) return;

    m_ctx->oformat = fmt;
    
    m_stream = av_new_stream(m_ctx, m_ctx->nb_streams); 
    if(!m_stream) return;

    //init video
    avcodec_get_context_defaults2(m_stream->codec, CODEC_TYPE_VIDEO);
    m_video_enc = m_stream->codec;

    CodecID codec_id = av_guess_codec(m_ctx->oformat, NULL, NULL, NULL, CODEC_TYPE_VIDEO);
    m_video_enc->codec_id = codec_id;

    AVCodec *codec;
    codec = avcodec_find_encoder(codec_id);
    if (!codec) return;

    m_video_enc->width = width;
    m_video_enc->height = height;
    m_video_enc->time_base.den = fps * 10000;
    m_video_enc->time_base.num = 10000;
    
    m_video_enc->pix_fmt = PIX_FMT_BGRA;
    if (codec && codec->pix_fmts) 
    {
        const enum PixelFormat *p= codec->pix_fmts;
        for (; *p!=-1; p++) {
            if (*p == m_video_enc->pix_fmt)
                break;
        }
        if (*p == -1)
            m_video_enc->pix_fmt = codec->pix_fmts[0];
    }

    if(m_video_enc->pix_fmt != PIX_FMT_BGRA)
    {
      //this codec needs colorplane conversion
      int sws_flags = SWS_BICUBIC;
      m_img_resample_ctx = sws_getContext(
                                     width,
                                     height,
                                     PIX_FMT_BGRA,
                                     width,
                                     height,
                                     m_video_enc->pix_fmt,
                                     sws_flags, NULL, NULL, NULL);

      if ( avpicture_alloc( (AVPicture*)&m_cvtpic, m_video_enc->pix_fmt,
                          m_video_enc->width, m_video_enc->height) )
        return;
    }

    m_video_enc->bit_rate = bitrate*1024;
    m_video_enc->gop_size = 12; /* emit one intra frame every twelve frames at most */

    // some formats want stream headers to be separate 
    if(m_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        m_video_enc->flags |= CODEC_FLAG_GLOBAL_HEADER; 

    m_video_enc->max_qdiff = 3; // set the default maximum quantizer difference between frames
    m_video_enc->thread_count = 1; // set how many thread need be used in encoding
    m_video_enc->rc_override_count = 0; // set ratecontrol override to 0
    if (!m_video_enc->rc_initial_buffer_occupancy) 
    {
        m_video_enc->rc_initial_buffer_occupancy = m_video_enc->rc_buffer_size*3/4; // set decoder buffer size
    }
    m_video_enc->me_threshold = 0; // set motion estimation threshold value to 0
    m_video_enc->intra_dc_precision = 0;
    m_video_enc->strict_std_compliance = 0;
    m_ctx->preload = (int)(0.5 * AV_TIME_BASE);
    m_ctx->max_delay = (int)(0.7 * AV_TIME_BASE);
    m_ctx->loop_output = -1;
    
    m_ctx->timestamp = 0;

    av_log_set_callback(ffmpeg_avcodec_log);
    av_log_set_level(AV_LOG_ERROR);

    if (avcodec_open(m_video_enc, codec) < 0) 
    {
      return;
    }

    //init audio
    if(abitrate)
    {
      m_astream = av_new_stream(m_ctx, m_ctx->nb_streams); 
      if(!m_astream) return;

      avcodec_get_context_defaults2(m_astream->codec, CODEC_TYPE_AUDIO);
      m_audio_enc = m_astream->codec;

      //use the format's default audio codec
      CodecID codeca_id = av_guess_codec(m_ctx->oformat, audioformat, NULL, NULL, CODEC_TYPE_AUDIO);
      m_audio_enc->codec_id = codeca_id;

      AVCodec *acodec;
      acodec = avcodec_find_encoder(codeca_id);
      if (!acodec) return;

      m_audio_enc->bit_rate = abitrate*1024;
      m_audio_enc->sample_rate = asr;
      m_audio_enc->channels = ach;

      if (avcodec_open(m_audio_enc, acodec) < 0) return;
    }

    AVFormatParameters params, *ap = &params;
    memset(ap, 0, sizeof(*ap));
    if (av_set_parameters(m_ctx, ap) < 0) return;
  
    url_open_dyn_buf(&m_ctx->pb);
    av_write_header(m_ctx);
    
    int size = width * height;
    m_bit_buffer_size = 1024 * 256;
    m_bit_buffer_size= FFMAX(m_bit_buffer_size, 4*size);

    m_bit_buffer = (uint8_t*)av_malloc(m_bit_buffer_size);

    m_init = 1;
  }
  ~WDL_VideoEncode()
  {
    if(m_stream && m_stream->codec) avcodec_close(m_stream->codec);     
    if(m_astream && m_astream->codec) avcodec_close(m_astream->codec);     
    av_free(m_bit_buffer);
    av_free(m_cvtpic.data[0]);
    av_free(m_ctx);
  }
  
  int isInited() { return m_init; }

  void encodeVideo(const LICE_pixel *buf)
  {
    if(m_img_resample_ctx)
    {
      //convert to output format
      uint8_t *p[1]={(uint8_t*)buf};
      int w[1]={m_video_enc->width*4};
      sws_scale(m_img_resample_ctx, p, w,
              0, m_video_enc->height, m_cvtpic.data, m_cvtpic.linesize);
    }
    int ret = avcodec_encode_video(m_video_enc, m_bit_buffer, m_bit_buffer_size, &m_cvtpic);
    if(ret>0)
    {
      AVPacket pkt;
      av_init_packet(&pkt);
      pkt.stream_index = 0;
      pkt.data = m_bit_buffer;
      pkt.size = ret;
      if (m_video_enc->coded_frame->pts != AV_NOPTS_VALUE)
            pkt.pts= av_rescale_q(m_video_enc->coded_frame->pts, m_video_enc->time_base, m_stream->time_base);
      if(m_video_enc->coded_frame->key_frame)
        pkt.flags |= PKT_FLAG_KEY; 
      av_interleaved_write_frame(m_ctx, &pkt);
    }
  }

  void encodeAudio(short *data, int nbsamples)
  {
    AVPacket pkt;
    int l = nbsamples;
    int fs = m_audio_enc->frame_size*m_audio_enc->channels;
    while(l>=fs)
    {
      av_init_packet(&pkt);
      pkt.size= avcodec_encode_audio(m_audio_enc, m_bit_buffer, m_bit_buffer_size, data); 
      if (m_audio_enc->coded_frame->pts != AV_NOPTS_VALUE)
        pkt.pts= av_rescale_q(m_audio_enc->coded_frame->pts, m_audio_enc->time_base, m_astream->time_base);
      pkt.flags |= PKT_FLAG_KEY;
      pkt.stream_index = 1;
      pkt.data = m_bit_buffer;
      av_interleaved_write_frame(m_ctx, &pkt);

      data += fs;
      l -= fs;
    }
  }

  int getBytes(unsigned char *p, int size)
  {
    //looks like there's no other way to get data from ffmpeg's dynamic buffers apart from closing them
    uint8_t *pb_buffer;
    int l = url_close_dyn_buf(m_ctx-> pb, &pb_buffer);
    if(l > 0)
    {
      m_queue.Add(pb_buffer, l);
      av_free(pb_buffer);
    }
    url_open_dyn_buf(&m_ctx->pb); //sets up next dynamic buffer for ffmpeg

    int s = min(size, m_queue.GetSize());
    if(s)
    {
      memcpy(p, m_queue.Get(), s);
      m_queue.Advance(s);
      m_queue.Compact();
    }
    return s;
  }

  void close()
  {
    av_write_trailer(m_ctx);
    uint8_t *pb_buffer;
    int l = url_close_dyn_buf(m_ctx-> pb, &pb_buffer);
    if(l)
    {
      m_queue.Add(pb_buffer, l);
      av_free(pb_buffer);
    }
  }

  //useful to get debugging information from ffmpeg
  static void ffmpeg_avcodec_log(void *ptr, int val, const char * msg, va_list ap)
  {
    AVClass* avc= ptr ? *(AVClass**)ptr : NULL;
    vprintf(msg, ap);
  }

protected:
  int m_init;

  AVFormatContext *m_ctx;
  AVStream *m_stream, *m_astream;
  AVCodecContext *m_video_enc, *m_audio_enc;
  struct SwsContext *m_img_resample_ctx;
  AVFrame m_cvtpic;
  uint8_t *m_bit_buffer;
  int m_bit_buffer_size;

  WDL_Queue m_queue;
};

#endif
