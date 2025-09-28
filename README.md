
# Original source from LavaRnd
* Landon Curt Noll -- http://www.isthe.com/
* Simon Cooper -- https://sfik.com/

For more info on LavaRnd see:
        http://www.LavaRnd.org/

# This code was re-invented with Grok
# I am am including the same license as the original.
# I have run diehard on outputs but I have NOT reviewed the "Digital Blender" for correctness.

* Cover your webcam with tape or lens cover so it is completely dark.
* Check permissions on the default video device: `ls -l /dev/video0`
* Your user probably needs to be in the video group `sudo usermod -aG video $USER`
* Then logout/login (or run `newgrp video` which starts a fresh shell.)


Example: `./lavarnd -s -l 1024`
* This will use generate 1024 bytes (printed as hex) and print some data statistics.
* The random bytes (in whatever format) will be written to STDOUT
while the "user outputs" are always on STDERR.
Therefore, it is safe to say `./lavarnd -s -l 4096 -t raw > rand.bin`

