OBJS = main.o ./lib/stb_image.o helpers.o ./lib/tiny_obj_loader.o 
CXXFLAGS = -g -Wall 
LDFLAGS = -lGL -lglfw -lGLEW -pthread -g 

all: ${OBJS}
	${CXX} ${OBJS}  ${LDFLAGS} -o view

clean:
	rm -f view ${OBJS}

test:	all
	./view

${OBJS} : helpers.h
