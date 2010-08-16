<?

// some path that you will be able to create
$config_temp_dir = "/tmp/username_stream_data"; 

$config_default_stream = "test"; // if no stream specified, use this one


// allowed servers, with their name, password, optional listen password
$config_streams = array(
  "test" => array("password" => "somePassword", "listen_password" => "",),

);


// performance tweaks ( safe to leave these as is for now )

$config_num_files = 4; 
$config_file_timeout = 30;

?>
