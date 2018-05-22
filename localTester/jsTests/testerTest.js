function main(args){
    userIo.writeString("Test Input Required");
    var inputTest = userIo.readString();
    userIo.writeString(inputTest + irdaCommands.IRDA_COMMAND_GET_BYTE);
}
