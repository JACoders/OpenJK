#!/bin/bash

/bin/ps ux > ./.temp 
alive=`grep -l 'openjkded.i386' ./.temp | wc -l`;
     
if [ $alive -eq 0 ]
then
     
     	lastrestart=`/bin/date`;
        echo "Server restarted on $lastrestart" ;        
        
        nohup ./openjkded.i386 \
                +set dedicated 2 \
                +set fs_game base \
                +exec server.cfg > echo;
            
        sleep 10;
        
        nohup ./start_server.sh &
fi