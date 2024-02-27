from nicegui import ui
import json
import pathlib


strFile = "wc_test_4x4.json"
strFile = "wc_default.json"

arrButtons = None
print(arrButtons)



class ToggleButton(ui.button):

    def __init__(self, *args, **kwargs):
        self._state = False
        if "idx" in kwargs.keys():
           self._idx = kwargs["idx"]
           del kwargs["idx"]
        super().__init__(*args, **kwargs)
        #self.on('click', self.toggle)


    def toggle(self) -> None:
        """Toggle the button state."""
        self._state = not self._state
        self.update()

    def setOff(self) -> None:
        self._state = False
        self.update()


    def update(self):
        #self._state = False
        self.props(f'color={"black" if not self._state else "green"}')
        super().update()

class BindValues:
    def __init__(self):
        self.strSelectedIdx = ""

bindValues = BindValues()

def LoadWordClockJson(strFileName):
   dictWorcClockConfig = None
   with open(str(pathlib.Path(__file__).parent.resolve()) + "/" + strFileName) as f:
       dictWorcClockConfig = json.load(f)
   return dictWorcClockConfig
   
   
def CheckWatchFace(arrWatchface):
   width = len(arrWatchface[0])   
   for row in arrWatchface:
      if len(row) != width:
         raise RuntimeError("Size mismatch in row:" + row)
   
         

def CreateMeanderMap(arrWatchface):
   # create idx array
   arrIdx = []
   iSize = len(arrWatchface)
   iStart = 0
   for x in range(iSize):
      arrIdx.append([i for i in range(iStart,iSize+iStart)])
      iStart += iSize
   
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

def CbToggleButton(idx, button: ui.button):
   button.toggle()
   arr = []
   for b in arrButtons:
      if b._state:
         arr.append(b._idx)
   arr.sort()
   bindValues.strSelectedIdx = str(arr)

def CbClearAll():
   for b in arrButtons:
      b.setOff()
   
def main():
   global arrButtons
   confiWc = LoadWordClockJson(strFile)
   arrOrig = confiWc["apperance"]["watchface"]
   CheckWatchFace(arrOrig)
   arrMap = CreateMeanderMap(arrOrig)

 
   print("Create html")
   ## show watch face in web frontend
   with ui.grid(columns=len(arrOrig[0])):
      iIdx = 0
      for row in arrOrig:
         for letter in row:
            btTmp = ToggleButton(letter, on_click=lambda e, idButton=arrMap[iIdx]: CbToggleButton(idButton, e.sender), idx = arrMap[iIdx]).tooltip(str(arrMap[iIdx]))
            if arrButtons == None:
               arrButtons = [btTmp]
            else:
               arrButtons.extend([btTmp])
            
            iIdx += 1
           
   ui.button("Clear", on_click=lambda: CbClearAll())
   ui.input('Selected LEDs').bind_value(bindValues, "strSelectedIdx")

   ui.run()


if __name__ in {"__main__", "__mp_main__"}:
   main()


