for f in $1/*.csv; do
	./create_bin.py "$f" 
done
