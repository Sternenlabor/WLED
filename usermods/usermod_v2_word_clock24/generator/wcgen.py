from nicegui import ui
import json
import pathlib


def LoadWordClockJson(strFileName):
   dictWorcClockConfig = None
   with open(str(pathlib.Path(__file__).parent.resolve()) + "/" + strFileName) as f:
       dictWorcClockConfig = json.load(f)
   return dictWorcClockConfig
   



def CreateMeanderMap(arrWatchface):
   # create idx array
   arrIdx = []
   iSize = len(arrWatchface)
   iStart = 0
   for x in range(iSize):
      arrIdx.append([i for i in range(iStart,iSize+iStart)])
      iStart += iSize
   print(arrIdx)

   tmpWc = []
   print("0. Orig: ",arrIdx)
   # 1. reverse even rows (0,2,4)
   iIdx = 0
   for row in arrIdx:
      if (iIdx % 2) != 0:
         row = row[::-1]
      tmpWc.append(row)
      iIdx += 1
   print("1. Step: ", tmpWc)
   # 2. reverse rows last = first / first = last
   tmpWc = tmpWc[::-1]
   print("2. Step: ", tmpWc)
   # 3. flatten
   arrFlat = []
   for row in tmpWc:
      for letter in row:
         arrFlat.append(letter)
   print("3. Step: ", arrFlat)
   return arrFlat



def main():

   confiWc = LoadWordClockJson("wc_test_4x4.json")
   arrOrig = confiWc["apperance"]["watchface"]
   arrMap = CreateMeanderMap(arrOrig)
   
   print("Create html")
   ## show watch face in web frontend
   with ui.grid(columns=len(arrOrig[0])):
      iIdx = 0
      for row in arrOrig:
         for letter in row:
            ui.label(letter).tooltip(str(arrMap[iIdx]))
            iIdx += 1

   ui.run()


if __name__ in {"__main__", "__mp_main__"}:
   main()


