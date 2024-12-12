for i in $(find -name *.c); do
    cat $i | grep CAPLENGTH
done
