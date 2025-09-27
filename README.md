* Check permissions on the default video device:
`ls -l /dev/video0`
* Your user probably needs to be in the video group
`sudo usermod -aG video $USER`
* Then logout/login (or run `newgrp video` which starts a fresh shell.)



