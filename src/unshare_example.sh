#!/bin/bash

echo "demo UTS namespace"
      
sudo unshare --uts --fork /bin/bash -c "hostname container && echo \"Hostname: \$(hostname)\""

echo "demo PID namespace"

sudo unshare --uts --fork --pid /bin/bash -c "sleep 10 & ps auxf | grep [s]leep"

echo "demo PID namespace with mount"

sudo unshare --uts --fork --pid --mount /bin/bash -c "sleep 10 & ps auxf | grep [s]leep" 

echo "demo PID namespace with mount and proc"   

sudo unshare --uts --fork --pid --mount /bin/bash -c \
  'mount -t proc proc /proc && { sleep 10 & sleep_pid=$!; ps auxf | grep [s]leep; wait $sleep_pid; umount /proc; }'