from nicegui import ui
import json
import pathlib

confiWc = None
with open(str(pathlib.Path(__file__).parent.resolve()) + '/wc_test.json') as f:
    confiWc = json.load(f)


iSizeX = confiWc["apperance"]["size"]["x"]
iSizeY = confiWc["apperance"]["size"]["y"]

## add letters to watch face array
arrWatchface = []
# start at last row - first LED is in the left bottom corner
for idxRow in range(len(confiWc["apperance"]["watchface"])-1 , -1, -1):
   strRow = confiWc["apperance"]["watchface"][idxRow]
   # inervt string on each even number
   if idxRow % 2 != 0:
      strRow = strRow[::-1]
   for charWc in strRow:
      arrWatchface.extend(charWc)

print(arrWatchface)

print("Print html")
## show watch face in web frontend
with ui.grid(columns=iSizeX):
   arrWatchFaceInverted = arrWatchface[::-1]
   for iRow in range(0, iSizeY):
      rngRow = range(iRow * iSizeX, iRow * iSizeX + iSizeX)
      iSubPos = 0
      if iRow % 2 == 0:
         for el in arrWatchFaceInverted[iRow * iSizeX :iRow * iSizeX + iSizeX][::-1]:
            ui.label(el).tooltip(str(rngRow[iSubPos]) + " even")
            iSubPos += 1

      else:         
         print(range(iRow * iSizeX, iRow * iSizeX + iSizeX))
         for el in arrWatchFaceInverted[iRow * iSizeX:iRow * iSizeX + iSizeX]:
            ui.label(el).tooltip(str(rngRow[iSubPos]) + " odd")
            iSubPos += 1
         

ui.run()
