EXE=fat

build: $(EXE)
$(EXE):
	gcc ./fat.c -g -o $(EXE)

debug: $(EXE)
	/home/alexandre/github/gf/gf2 -nx --args ./$(EXE) ./main_floppy.img

run: $(EXE)
	./$(EXE) ./main_floppy.img

clean:
	cp ./main_floppy.img.bck ./main_floppy.img
	mcopy -i ./main_floppy.img ./test.txt "::test.txt"
	$(RM) $(EXE)
