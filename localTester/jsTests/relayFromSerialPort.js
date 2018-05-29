function main(args){
   //copy 100 chars from the serial port to user output
   userIo.writeStringJs("Started, wating on serial port");
   var string = "";
   userIo.writeStringJs("Blob is valid:" + jsSystem.validDependencyBlob(1.0).toString());
   userIo.writeStringJs("Bytes in FIFO before flush:" + serialPort.bytesAvailable().toString());
   serialPort.flushFifo();
   userIo.writeStringJs("Bytes in FIFO after flush:" + serialPort.bytesAvailable().toString());
   for(var count = 0; count < 100; count++){
      if(serialPort.bytesAvailable() > 0){
         var letter = String.fromCharCode(serialPort.receiveUint8());
         if(isAlphanumeric(letter))
            string += letter;
         else
            string += " ";
      }
      else{
         jsSystem.uSleep(16666);
      }
   }
   userIo.writeStringJs(string);
   userIo.writeStringJs("Ended");
}
