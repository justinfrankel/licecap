#include "lice.h"
#include "../ptrlist.h"
#include "../wdltypes.h"


#define OCTREE_DEPTH 5  // every depth level adds 3 bits of RGB colorspace (depth 8 => 24-bit RGB)

struct ONode
{
  WDL_INT64 colorcount;  // number of color instances at or below this node
  WDL_INT64 sumrgb[3];
  int childflag;   // 0=leaf, >0=index of single child, <0=branch
  int leafidx;     // populated at the end
  ONode* next;     // for OTree::branches
  ONode* children[8];
};

struct OTree
{
  int maxcolors;
  int leafcount;  
  ONode* trunk;
  ONode* branches[OCTREE_DEPTH];  // linked lists of branches for each level of the tree
  LICE_pixel* palette;  // populated at the end
};



int LICE_BuildPalette(LICE_IBitmap* bmp, LICE_pixel* palette, int maxcolors)
{
  void* tree = LICE_CreateOctree(maxcolors);
  LICE_BuildOctree(tree, bmp);
  int sz = LICE_ExtractOctreePalette(tree, palette);
  LICE_DestroyOctree(tree);
  return sz;
}


static void AddColorToTree(OTree*, unsigned char rgb[]);
static int FindColorInTree(OTree*, unsigned char rgb[]);
static int PruneTree(OTree*);
static void DeleteNode(OTree*, ONode*);
static int CollectLeaves(OTree*);
static int CollectNodeLeaves(ONode* node, LICE_pixel* palette, int colorcount);


void* LICE_CreateOctree(int maxcolors)
{
  OTree* tree = new OTree;
  memset(tree, 0, sizeof(OTree));
  tree->maxcolors = maxcolors;
  tree->trunk = new ONode;
  memset(tree->trunk, 0, sizeof(ONode));
  return tree;
}


void LICE_DestroyOctree(void* octree)
{
  OTree* tree = (OTree*)octree;
  DeleteNode(tree, tree->trunk);
  free(tree->palette);
  delete tree;
}


int LICE_BuildOctree(void* octree, LICE_IBitmap* bmp)
{
  OTree* tree = (OTree*)octree;
  if (!tree) return 0;

  if (tree->palette) 
  {
    free(tree->palette);
    tree->palette=0;
  }

  int x, y;
  for (y = 0; y < bmp->getHeight(); ++y)
  {
    LICE_pixel* px = bmp->getBits()+y*bmp->getRowSpan();
    for (x = 0; x < bmp->getWidth(); ++x)
    {    
      unsigned char rgb[3];
      rgb[0]=LICE_GETR(px[x]);
      rgb[1]=LICE_GETG(px[x]);
      rgb[2]=LICE_GETB(px[x]);
      AddColorToTree(tree, rgb);      
      if (tree->leafcount > tree->maxcolors) PruneTree(tree);
    }
  }

  return tree->leafcount;
}


int LICE_FindInOctree(void* octree, LICE_pixel color)
{
  OTree* tree = (OTree*)octree;
  if (!tree) return 0;

  if (!tree->palette) CollectLeaves(tree);
  
  unsigned char rgb[3];
  rgb[0] = LICE_GETR(color);
  rgb[1] = LICE_GETG(color);
  rgb[2] = LICE_GETB(color);
  return FindColorInTree(tree, rgb);
}


int LICE_ExtractOctreePalette(void* octree, LICE_pixel* palette)
{
  OTree* tree = (OTree*)octree;
  if (!tree || !palette) return 0;

  if (!tree->palette) CollectLeaves(tree);

  memcpy(palette, tree->palette, tree->maxcolors*sizeof(LICE_pixel));
  return tree->leafcount;
}



void LICE_TestPalette(LICE_IBitmap* bmp, LICE_pixel* palette, int numcolors)
{
  int x, y;
  for (y = 0; y < bmp->getHeight(); ++y)
  { 
    LICE_pixel* px = bmp->getBits()+y*bmp->getRowSpan();
    for (x = 0; x < bmp->getWidth(); ++x)
    {
      const LICE_pixel col = px[x];
      const int rgb[3] = { (int)LICE_GETR(col), (int)LICE_GETG(col), (int)LICE_GETB(col) };

      int minerr;
      int bestcol=-1;
      int i;
      for (i = 0; i < numcolors; ++i)
      {
        const LICE_pixel palcol = palette[i];
        const int rerr[3] = { rgb[0]-(int)LICE_GETR(palcol), rgb[1]-(int)LICE_GETG(palcol), rgb[2]-(int)LICE_GETB(palcol) };
        const int err = rerr[0]*rerr[0]+rerr[1]*rerr[1]+rerr[2]*rerr[2];
        if (bestcol < 0 || err < minerr)
        {
          bestcol=i;
          minerr=err;
        }
      }
      px[x] = palette[bestcol];
    }
  }
}


void AddColorToTree(OTree* tree, unsigned char rgb[3])
{
  ONode* p = tree->trunk;
  p->colorcount++;

  int i;
  for (i = OCTREE_DEPTH-1; i >= 0; --i)
  {
    int j = i+8-OCTREE_DEPTH;
    unsigned char idx = (((rgb[0]>>(j-2))&4))|(((rgb[1]>>(j-1))&2))|((rgb[2]>>j)&1);

    ONode* np = p->children[idx];
    bool isleaf = false;

    if (np)
    {
      isleaf = !np->childflag;
    }
    else // add node
    {
      if (!p->childflag) // first time down this path
      {
        p->childflag=idx+1;
      }
      else if (p->childflag > 0)  // creating a new branch
      {    
        p->childflag = -1;
        p->next = tree->branches[i];
        tree->branches[i] = p;
      }
      // else multiple branch, which we don't care about

      np = p->children[idx] = new ONode;
      memset(np, 0, sizeof(ONode));    
    }

    np->sumrgb[0] += rgb[0];
    np->sumrgb[1] += rgb[1];
    np->sumrgb[2] += rgb[2];
    np->colorcount++;

    if (isleaf) return;

    p=np; // continue downward
  }

  // p is a new leaf at the bottom
  tree->leafcount++;
}

int FindColorInTree(OTree* tree, unsigned char rgb[3])
{
  ONode* p = tree->trunk;

  int i;
  for (i = OCTREE_DEPTH-1; i >= 0; --i)
  { 
    if (!p->childflag) break;

    int j = i+8-OCTREE_DEPTH;
    unsigned char idx = (((rgb[0]>>(j-2))&4))|(((rgb[1]>>(j-1))&2))|((rgb[2]>>j)&1);

    ONode* np = p->children[idx];
    if (!np) break; 

    p = np;
  }

  return p->leafidx;
}


int PruneTree(OTree* tree)
{
  ONode* branch=0;
  int i;
  for (i = 0; i < OCTREE_DEPTH; ++i) // prune at the furthest level from the trunk
  {
    branch = tree->branches[i];
    if (branch)
    {
      tree->branches[i] = branch->next;
      branch->next=0;
      break;
    }
  }

  if (branch) 
  {
    int i;
    for (i = 0; i < 8; ++i)
    {
      if (branch->children[i])
      {
        DeleteNode(tree, branch->children[i]);
        branch->children[i]=0;
      }
    }
    branch->childflag=0; // now it's a leaf
    tree->leafcount++;
  }
  
  return tree->leafcount;
}

int CollectLeaves(OTree* tree)
{
  if (tree->palette) free(tree->palette);
  tree->palette = (LICE_pixel*)malloc(tree->maxcolors*sizeof(LICE_pixel));

  int sz = CollectNodeLeaves(tree->trunk, tree->palette, 0);
  memset(tree->palette+sz, 0, (tree->maxcolors-sz)*sizeof(LICE_pixel));

  return sz;
}

int CollectNodeLeaves(ONode* p, LICE_pixel* palette, int colorcount)
{  
  if (!p->childflag)
  {
    p->leafidx = colorcount;
    int r = (int)((double)p->sumrgb[0]/(double)p->colorcount);
    int g = (int)((double)p->sumrgb[1]/(double)p->colorcount);
    int b = (int)((double)p->sumrgb[2]/(double)p->colorcount);
    palette[colorcount++] = LICE_RGBA(r, g, b, 255);
  }
  else 
  {
    if (p->childflag > 0)
    {
      colorcount = CollectNodeLeaves(p->children[p->childflag-1], palette, colorcount);
    }
    else
    {
      int i;
      for (i = 0; i < 8; ++i)
      {
        if (p->children[i])
        {
          colorcount = CollectNodeLeaves(p->children[i], palette, colorcount);
        }
      }
    }
    // this is a branch or passthrough node,  record the index
    // of any downtree leaf here so that we can return it for
    // color lookups that want to diverge off this node 
    p->leafidx = colorcount-1;
  }

  // colorcount should == leafcount
  return colorcount;
}


void DeleteNode(OTree* tree, ONode* p)
{
  if (!p->childflag)
  {
    tree->leafcount--;
  }
  else if (p->childflag > 0)
  {
    DeleteNode(tree, p->children[p->childflag-1]);
  }
  else
  {
    int i;
    for (i = 0; i < 8; ++i)
    {
      if (p->children[i])
      {       
        DeleteNode(tree, p->children[i]);     
      }
    } 
  }

  delete p;
}

