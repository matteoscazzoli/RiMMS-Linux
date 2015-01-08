#!/bin/bash

./RiMMS.exe --kill
sleep 2
rm RiMMS.dbg
rm RiMMS.log
./RiMMS.exe --debug

