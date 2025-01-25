for i in $(find ./kernel/ -name *.c); do
    t=$(cat $i | grep malloc)
    if [ ! -z "$t" ]; then
        echo $i
    fi
done
