IGNMapper
=========

This program allows to automatically assemble maps in order to create GPS background maps in Garmin KMZ format.

In order to use it:

1. Create a directory named "maps" in which you put screenshots to be assembled. A minimum amount of overlap is needed between the different images in order to allow a robust match;
2. launch IGNMapper. It will read the directory and create a database of the images it finds and assign arbitrary coordinates to them
3. use the keys to move/place images together:
```
b: show/hide image borders
s: select the image that is highlighted
t: snap the current highlighted image onto the selected one using best match (if a match is found)
p: try to automatically fit all maps consistently (worksmost of the time)
d: compute and display descriptors for currently highlighted image (for debug purposes only)
e: show/hide descriptors computed with 'd'
w: save the database (including image positions)

```
Optionally, the program will load maps/mask.png and use it as a mask (white/transparent) to combine the images and remove unwanted features from the screenshots in the combined images.

4. Onces all images are consistency placed, you need to add two control points by using Shift+left mouse click. Each new control point erases the oldest one. Good control points should be diagonaly placed over the full set of images to ensure maximal accuracy;
5. save the map using 'w';
6. edit the file maps/file_map.xml and set the real GPS coordinates of the two control points. 
7. re-launch IGNMapper and hit 'g' to display the tile grid. Choose a proper view and window size (some GPS like GARMIN expect to find less than 100 tiles in custom maps) and press 'x' to export a kmz file. 

 
