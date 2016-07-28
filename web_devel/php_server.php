<?php
# server.php

$server = stream_socket_server("tcp://127.0.0.1:1337", $errno, $errorMessage);

if ($server === false) {
    throw new UnexpectedValueException("Could not bind to socket: $errorMessage");
}

$buffer='';
echo "Waiting for client connection(s)\n";
for (;;) {
    $client = @stream_socket_accept($server);

    if ($client) {
      //while ( strcmp('exit\\n', $buffer) ) {
      for (;;) {
        $buffer = '';
        while( !preg_match('/\n/', $buffer) )
          $buffer .= fread($client, 1); 
        echo $buffer;
      }
      echo 'Closing connection\n';
      fclose($client);
    }

}
