APP=cloudgps

CC = gcc

CFLAGS = -g -O2 -DN900 `pkg-config --libs glib-2.0` `pkg-config --cflags --libs liblocation sdl` -D_GNU_SOURCE
PKGCFLAGS = -Wall

LIBS = `pkg-config --libs glib-2.0` `pkg-config --cflags --libs liblocation sdl json-c libcurl` -lSDL_gles -lGLESv1_CM -lEGL -lm -lSDL_image -lSDL_ttf -lz

TARGET = cloudgps

SOURCES = src/main.c src/tile.c src/tileengine.c src/glunproject.c src/file.c src/texture.c src/downloadQueue.c \
src/console.c src/screen.c src/network.c \
src/geocoding/geocoder.c src/geocoding/cloudmade_geocoder.c src/geocoding/google_geocoder.c \
src/geocoding/google_maps_geocoder.c src/geocoding/coordinate_reader.c \
src/routing/router.c src/routing/google_maps_router.c src/routing/cloudmade_router.c\
src/markerIterator.c src/navigation/navigation.c src/unicode2ascii.c src/geometry.c

OBJS = $(SOURCES:%.c=%.o)

all:	$(TARGET)

$(TARGET): $(OBJS)
	rm -f $(TARGET)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) -o $@ -c $^ $(CFLAGS) $(PKGCFLAGS)

clean:
	rm -f $(TARGET) $(OBJS)

install:
	@echo You must be root to install
	install -d $(DESTDIR)/usr/share/cloudgps/res/
	mkdir $(DESTDIR)/usr/bin/
	install cloudgps $(DESTDIR)/usr/bin/
	install res/*.png $(DESTDIR)/usr/share/cloudgps/res/
	install res/*.ini $(DESTDIR)/usr/share/cloudgps/
	install -d $(DESTDIR)/usr/share/applications/hildon/
	install res/*.desktop $(DESTDIR)/usr/share/applications/hildon/
	for s in 40 26; do \
	  install -d $(DESTDIR)$(prefix)/usr/share/icons/hicolor/$${s}x$${s}/hildon ;\
	  install -m 644 res/$(APP).$$s.png $(DESTDIR)$(prefix)/usr/share/icons/hicolor/$${s}x$${s}/hildon/$(APP).png ;\
	done
	install -d $(DESTDIR)$(prefix)/usr/share/icons/hicolor/scalable/hildon
	install -m 644 res/$(APP).64.png $(DESTDIR)$(prefix)/usr/share/icons/hicolor/scalable/hildon/$(APP).png
	

