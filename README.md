iStuffTracking
==============

DIP Project

Recognition and tracking for low-performance devices.

Usage
-----

### To compile (requires Boost and OpenCV):

```sh
cd dbg
make
```
  
### After **compiling**:
  
* First execution:
  `./iStuffTracking --database databaseName --folder folderPath`

  Folder path must contain some images and their relative *.lbl* file, formatted as follows:
  ```
  LabelName1
  x1 y1
  LabelName2
  x2 y2
  ...
  ```
* Successive executions:
  `./iStuffTracking --database databaseName`
