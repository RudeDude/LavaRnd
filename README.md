
# Original source from LavaRnd
* Landon Curt Noll -- http://www.isthe.com/
* Simon Cooper -- https://sfik.com/

For more info on LavaRnd see:
        http://www.LavaRnd.org/

# This code was re-invented with Grok
# I am am including the same license as the original.
# I have run diehard on outputs but I have NOT reviewed the "Digital Blender" for correctness.

* Check permissions on the default video device:
`ls -l /dev/video0`
* Your user probably needs to be in the video group
`sudo usermod -aG video $USER`
* Then logout/login (or run `newgrp video` which starts a fresh shell.)


Example: ./lavarnd -s -l 1024 
* This will use generate 1024 bytes and print some data statistics.


