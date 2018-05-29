function main(args){
   //copy 100 chars from the serial port to user output
   userIo.writeStringJs("Started, wating on serial port");
   var string;
   for(var count = 0; count < 100; count++){
      if(serialPort.bytesAvailable() > 0){
         var letter = serialPort.receiveUint8();
         if(isAlphanumeric(letter))
            string += String.fromCharCode(letter);
      }
      else{
         jsSystem.uSleep(16666);
      }
   }
   userIo.writeStringJs(string);
   userIo.writeStringJs("Ended");
}
