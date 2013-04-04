sox %1 -w -r 22050 -t .wav __temp__.wv
lipsyncthing __temp__.wv %~dpn1.lip
xbadpcmencode __temp__.wv %~dpn1.wxb
del __temp__.wv

