CPP = gcc
TARGET = botaim_mm
ARCHFLAG = i686
BASEFLAGS = -Dstricmp=strcasecmp -Dstrcmpi=strcasecmp
OPTFLAGS = -DNDEBUG
CPPFLAGS = ${BASEFLAGS} ${OPTFLAGS} -march=${ARCHFLAG} -mtune=generic -mmmx -msse -msse2 -O2 -m32 -mfpmath=sse -w \
-I"../metamod-p/metamod" -I"../hlsdk-2.3-p4/multiplayer/common" -I"../hlsdk-2.3-p4/multiplayer/dlls" -I"../hlsdk-2.3-p4/multiplayer/engine" -I"../hlsdk-2.3-p4/multiplayer/pm_shared"

OBJ = 	${TARGET}.o

${TARGET}.so: ${OBJ}
	${CPP} -fPIC -shared -o $@ ${OBJ} -Xlinker -Map -Xlinker ${TARGET}.map -ldl
	mv *.o Release
	mv *.map Release
	mv $@ Release

clean:
	rm -f Release/*.o
	rm -f Release/*.map

distclean:
	rm -rf Release
	mkdir Release	

%.o:	%.cpp
	${CPP} ${CPPFLAGS} -c $< -o $@

%.o:	%.c
	${CPP} ${CPPFLAGS} -c $< -o $@
