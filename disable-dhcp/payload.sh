#!/bin/sh
hacked_sbin_path=/tmp/hacked_sbin

mkdir "$hacked_sbin_path"

for f in /usr/sbin/*
do
    # The powerline system has no basename command
    basename="${f##*/}"
    # Copy file $f
    # The powerline has no cp command
    cat "$f" > "$hacked_sbin_path"/"${basename}"
    chmod +x "$hacked_sbin_path"/"${basename}"
done

echo '#!/bin/sh
echo $$ > /tmp/udhcpd.pid
' > "$hacked_sbin_path"/udhcpd

killall udhcpd

mount --bind "$hacked_sbin_path" /usr/sbin
echo "\n\n\n"
echo Done
