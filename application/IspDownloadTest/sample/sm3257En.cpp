/*
* Copyright 2011, Silicon Motion Technology Corp.
*
*/
#include "StdAfx.h"
#include "SMIFunc.h"
#include "sm3257En.h"
#include "ForceModeBuffer.h"

/*
 * Definitions of Global Parameters
*/
int  SM3257Enx_FCE;
int  SM3257Enx_FMU;
int  SM3257Enx_BlockSize;
int  SM3257Enx_PageSize;
int  SM3257Enx_TWIN;
BYTE SM3257Enx_DataBuffer[512*(16*2+1)];
BYTE FlashIDBuf[512];
BYTE SFRBuf[512];
//BOOL DoUpdateStrongTable( pUSB_DEV_HANDLE udevh, BYTE*	nStrongTable, BOOL  bEnableStrong);

DWORD nLISPBlock;
BYTE nPageSize;
BOOL SMI_DoEraseISPBeforeLoadISP(pUSB_DEV_HANDLE udev);
BOOL SMI_DoReadSFRBuf_PageSize(pUSB_DEV_HANDLE udev);

int SMI_DoReset(pUSB_DEV_HANDLE udev);
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
/*
 *Update the Unique Serial Number
*/
int DoUpdateUFDSerialNumber(BYTE *buf , char* szSerial)
{
 	int nlength,i;
 	nlength = strlen(szSerial);
 	//clear the original Serial Number
 	memset(buf+0xB4 , 0x00 , buf[0xB2]-2);
 	
 	//fill the new serial number's information
 	buf[0xB2] = nlength*2+2;
 	for(i = 0 ; i < nlength ; i++){
	 	buf[0xB4+i*2]=szSerial[i];
	}
	return nlength;
}

void OutputParam(char *szPreString , int nValue){
	printf("%s : %d\n",szPreString , nValue);
}
void OutputParams(char *szPreString , char *szValue){
	printf("%s : %s\n",szPreString , szValue);
}

/*
 * Read the FlashIDBuffer for Checking some information
*/
int SMI_DoReadFlashIDBuffer(pUSB_DEV_HANDLE udev ){
	BYTE cmd[16];
	memset(cmd, 0 , sizeof(cmd));
	cmd[0x00] = 0xF0;
//	cmd[0x01] = 0x20;
	cmd[0x01] = 0x06;
	cmd[0x0B] = 0x01;
//	cmd[0x0C] = 0x07;
	return usb_scsi_read(udev , FlashIDBuf ,512, cmd, 0x10);
}

/*
 * Send the Pretest command
*/
int SMI_DoPretestA(pUSB_DEV_HANDLE udev, BYTE *write_bufA , int nFileSizeA)
{
	int nRtnValue=false;
	BYTE cmd[16];

	memset(cmd, 0 , sizeof(cmd));
	cmd[0x00] = 0xF1;
	cmd[0x01] = 0x04;
	cmd[0x02] = 0xD4;
	cmd[0x03] = 0x00;
	cmd[0x04] = 0x00;
	cmd[0x0B] = 0x01;

	//if(nFileSizeA>0){
		nRtnValue = usb_scsi_write(udev , write_bufA , nFileSizeA , cmd , 0x10);
		if(nRtnValue){
			
		}
	//}

/*
	if(nFileSize>0){
		nRtnValue = usb_scsi_write(udev , PretestBuf+0x10 , nFileSize-0x10 , PretestBuf , 0x10);
		if(nRtnValue){
			
		}
	}
*/	
	return nRtnValue;
}

int SMI_DoPretestB(pUSB_DEV_HANDLE udev, BYTE *write_bufB , int nFileSizeB)
{
	int nRtnValue=false;
	BYTE cmd[16];

	memset(cmd, 0 , sizeof(cmd));
	cmd[0x00] = 0xF1;
	cmd[0x01] = 0x04;
	cmd[0x02] = 0xC0;
	cmd[0x03] = 0x00;
	cmd[0x04] = 0x00;
	cmd[0x0B] = 0x01;

	//if(nFileSizeB>0){
		nRtnValue = usb_scsi_write(udev , write_bufB , nFileSizeB , cmd , 0x10);
		if(nRtnValue){
			
		}
	//}

	return nRtnValue;
}


int SMI_DoPretest(pUSB_DEV_HANDLE udev, BYTE *write_buf , int nFileSize)
{
	int nRtnValue=false;
	BYTE cmd[16],i;

	printf("Pretest : ");	
	for(i=0;i<3;i++){
		memset(cmd, 0 , sizeof(cmd));
		cmd[0x00] = 0xF1;
		cmd[0x01] = 0x0A;
		cmd[0x02] = 0xA0;
		cmd[0x03] = 0x00;
		cmd[0x04] = 0x00;
		cmd[0x0B] = 16;

	//if(nFileSize>0){
		nRtnValue = usb_scsi_write(udev , write_buf+(i*8*1024), 8*1024 , cmd , 0x10);
		if(nRtnValue){
			
		}
	//}
		printf(" %X ", i);	
	}
	printf("\r\n");	
	return nRtnValue;
}




DWORD MyPow(int nBase , int npow){
	DWORD nVal=1;
	int i;
	for( i = 0 ; i < npow ; i++){
		nVal*= nBase;
	}
	return nVal;
}

/*
 * To Download ISP
*/
int SMI_DoDownloadISP( pUSB_DEV_HANDLE udev , BYTE* ISPBuf , int nFileSize, BYTE* StrongBuf)
{
	BOOL bRtnValue=false;
	int i,nDownloadCnt=0,nISPSector,nPrevSysIdx=4;
	UCHAR Command[16];
	UCHAR *ISPProcPtr;
	//UINT *ISPProcPtr;

//UINT nLISPBlock;
//BYTE nPageSize;

	//Read Flash Page Size
	SMI_DoReadSFRBuf_PageSize(udev);
	//Erase ISP
	SMI_DoEraseISPBeforeLoadISP(udev);
	//
	nISPSector = nFileSize/512;
	if(nFileSize%512)  nISPSector++;
	nDownloadCnt = (nISPSector-nPrevSysIdx)/nPageSize+2;
	if((nISPSector-nPrevSysIdx)%(nPageSize))  nDownloadCnt++;

	ISPProcPtr = ISPBuf;
	printf("ISP Page : ");
			for(i = 0 ; i < nDownloadCnt ; i++){
				memset(Command,0,sizeof(Command));
				Command[0x00] = 0xF1;
				Command[0x01] = 0x01;
				Command[0x02] = (BYTE)(nLISPBlock>>8);
				Command[0x03] = (BYTE)(nLISPBlock);
				Command[0x04] = (BYTE)StrongBuf[2*i+1];
				Command[0x05] = (BYTE)StrongBuf[2*i];
				if(SFRBuf[0x124]&0x20){//Enable Randomizer
					Command[0x07] |= 0x30;
				}
				else{
					Command[0x07] |= 0x10;
				}
				switch(i){
				case 0:
					Command[0x09] = 0x7E;//enable ecc-48bits Write(TARG7-bit3)
					Command[0x0A] = 0x80;//TARG8-bit7 Without Spare program
					Command[0x0B] = 0x02;
					break;
				case 1:
					Command[0x08] = 0xE4;
					Command[0x09] = 0x32;
					Command[0x0B] = 0x02;
					memset(Command+0x0C , 0xE4 , 3);
					break;
				default:
					Command[0x08] = 0xE4;
					Command[0x09] = 0x32;
					Command[0x0B] = nPageSize;
					if(i == (nDownloadCnt-1) && (nISPSector-nPrevSysIdx)%(nPageSize))
						Command[0x0B] = (nISPSector-nPrevSysIdx)%nPageSize;
					memset(Command+0x0C , 0xE4 , 3);
					break;
				}
				Command[0x0F] = i;
				printf(" %X ", 	Command[0x05]);	
				//bRtnValue = SCSICommandWrite(lp , Command , ISPProcPtr , Command[0x0B]*512);
				bRtnValue = usb_scsi_write(udev , ISPProcPtr, Command[0x0B]*512 , Command , 0x10);
				if(!bRtnValue)
					break;
				ISPProcPtr+=(Command[0x0B]*512);
			}
	printf("\r\n");	

	return bRtnValue;
}

/*
int SMI_DoWriteCID(pUSB_DEV_HANDLE udev , BYTE *CIDBuf , int nFileSize){
	int nRtnValue=0, i=0;
	BYTE ReadBuf[2048];
	CHAR szSerial[]="AABBCCDDEEFF0000";
	BYTE cb[16];
	memset(cb,0,sizeof(cb));
	cb[0x00] = 0xF1;
	cb[0x01] = 0x03;
	cb[0x0b] = 0x04;
	DoUpdateUFDSerialNumber(CIDBuf , szSerial);
	if(usb_scsi_write(udev , CIDBuf , cb[0x0b]*512 , cb , 0x10)){
		//read for compare
		memset(cb,0,sizeof(cb));
		cb[0x00] = 0xf0;
		cb[0x01] = 0x02;
		cb[0x0b] = 0x04;
		memset(ReadBuf,0,sizeof(ReadBuf));
		if(usb_scsi_read(udev , ReadBuf , cb[0x0b]*512 , cb , 0x10)){
			if(memcmp(CIDBuf, ReadBuf,nFileSize)==0){
				nRtnValue=1;
			}
			else
			{
				for(i=0;i<sizeof(ReadBuf);i++) 
				{
					if(CIDBuf[i]!=ReadBuf[i])
					{
						printf("Error on(%d):%X-%X\r\n", i, CIDBuf[i], ReadBuf[i]);						
					}
				}	
			}
		}
	}
	return nRtnValue;
}
*/

BOOL SMI_DoClearSystemBlocks(pUSB_DEV_HANDLE udev)
{
	BOOL bRtnValue=true;
	BYTE Command[16];

	SMI_DoReadFlashIDBuffer(udev);

	//Erase ISP
	if((FlashIDBuf[0x102]==0xff) && (FlashIDBuf[0x103]==0xff)){
	}else{
		memset(Command,0,sizeof(Command));
		Command[0x00] = 0xF0;	
		Command[0x01] = 0x05;
		Command[0x02] = FlashIDBuf[0x102];
		Command[0x03] = FlashIDBuf[0x103];
	    Command[0x06] = 0x55;//for Inform ISP to open erase command			
		bRtnValue=usb_scsi_read(udev , SM3257Enx_DataBuffer,0,Command,0x10);
	}
	//Erase Index
	if((FlashIDBuf[0x104]==0xff) && (FlashIDBuf[0x105]==0xff)){
	}else{
		memset(Command,0,sizeof(Command));
		Command[0x00] = 0xF0;	
		Command[0x01] = 0x05;
		Command[0x02] = FlashIDBuf[0x104];
		Command[0x03] = FlashIDBuf[0x105];
	    Command[0x06] = 0x55;//for Inform ISP to open erase command			
		bRtnValue=usb_scsi_read(udev , SM3257Enx_DataBuffer,0,Command,0x10);
	}
	//Erase Spare
	if((FlashIDBuf[0x106]==0xff) && (FlashIDBuf[0x107]==0xff)){
	}else{
		memset(Command,0,sizeof(Command));
		Command[0x00] = 0xF0;	
		Command[0x01] = 0x05;
		Command[0x02] = FlashIDBuf[0x106];
		Command[0x03] = FlashIDBuf[0x107];
	    Command[0x06] = 0x55;//for Inform ISP to open erase command			
		bRtnValue=usb_scsi_read(udev , SM3257Enx_DataBuffer,0,Command,0x10);
	}

	//Erase Temp ISP
	if((FlashIDBuf[0x110]==0xff) && (FlashIDBuf[0x111]==0xff)){
	}else{
		memset(Command,0,sizeof(Command));
		Command[0x00] = 0xF0;	
		Command[0x01] = 0x05;
		Command[0x02] = FlashIDBuf[0x110];
		Command[0x03] = FlashIDBuf[0x111];
	    Command[0x06] = 0x55;//for Inform ISP to open erase command			
		bRtnValue=usb_scsi_read(udev , SM3257Enx_DataBuffer,0,Command,0x10);
	}

	printf("Erase Info/ISP/Index Block : %X%X %X%X %X%X\r\n",FlashIDBuf[0x100],FlashIDBuf[0x101],FlashIDBuf[0x102],FlashIDBuf[0x103],FlashIDBuf[0x104],FlashIDBuf[0x105]);

	SMI_DoReset(udev);
	sleep(1);
	return bRtnValue;
} 
 
BOOL SMI_DoEraseISPBeforeLoadISP(pUSB_DEV_HANDLE udev)
{
	BOOL bRtnValue=true;
	BYTE Command[16];

	SMI_DoReadFlashIDBuffer(udev);
	nLISPBlock=(FlashIDBuf[0x102]<<8)+FlashIDBuf[0x103];

	if((FlashIDBuf[0x102]==0xff) && (FlashIDBuf[0x103]==0xff)){

	}else{	
		memset(Command,0,sizeof(Command));
		Command[0x00] = 0xF0;
		Command[0x01] = 0x05;
		Command[0x02] = FlashIDBuf[0x102];
		Command[0x03] = FlashIDBuf[0x103];
		Command[0x06] = 0x55;//For Inform ISP to opened the erase command
		bRtnValue=usb_scsi_read(udev , SM3257Enx_DataBuffer,0,Command,0x10);

		printf("ISP Block : %X%X \r\n",FlashIDBuf[0x102],FlashIDBuf[0x103]);
	}

	return bRtnValue;
}

BOOL SMI_DoReadSFRBuf_PageSize(pUSB_DEV_HANDLE udev)
{
	BOOL bRtnValue=true;
	BYTE Command[16];

	Command[0x00] = 0xF0;
	Command[0x01] = 0x03;
	Command[0x0B] = 0x01;
	bRtnValue=usb_scsi_read(udev , SFRBuf,512,Command,0x10);

	nPageSize = SFRBuf[0x80];
	printf("Page Size : %d \r\n",nPageSize);

	return bRtnValue;
}

/*
 * Send a software reset command to the SM3257En
 */
int SMI_DoReset(pUSB_DEV_HANDLE udev)
{
	BYTE cmd[16];
	int ret;

	memset(cmd, 0, sizeof(cmd));

	/* Reset command */
	cmd[0x00] = 0xf0;
	cmd[0x01] = 0x2c;

	ret = usb_scsi_read(udev, NULL, 0, cmd, 0x10);

	/* Wait for device to come back */
	sleep(1);

	return 0;
}


/*
 * Read a file into a data buffer for programming
 */
int read_file(char * filename, BYTE *data )
{
	int len;
	int total_len = 0;
	
	FILE *fd = fopen(filename, "rb");
	if (fd == NULL){
		//OutputParam("nFileSize",nFileSize);
		debug("%s: Can't read %s\n", __func__, filename);
		return -1;
	}

	/* Read in entire file up til MAX_READ_SIZE */
	do {
		len = fread(data++, 1, 1, fd);
		total_len += len;
	} while (len && (total_len < MAX_READ_SIZE));

	fclose(fd);
	
	  debug("%s: Read %d byte from %s\n", __func__, total_len, filename);
	

	return total_len;
}


/*
 * Write a data buffer out to a file
 */
int write_file(char * filename, BYTE *data, int len)
{
	int ret;

	FILE *fd = fopen(filename, "wb");
	if (fd == NULL) {
		printf("ERROR: could not write to file %s\n", filename);
		return -1;
	}

	debug("%s: writing %d byte to %s\n", __func__, len, filename);

	ret = fwrite(data, 1, len, fd);
	fclose(fd);

	return ret;
}


/*
 * Show usage help
 */
void usage(void)
{	
	printf("sm3257En_prog [options] <command>\n");
	printf("  version: %s\n", VERSION);
	printf("\n");
	printf("  valid options:\n");
	printf("\t-p <pretest filename>\n");
//	printf("\t-f <flash/ISP filename>\n");
	printf("\t-i <ISP filename>\n");
//	printf("\t-c <CID filename>\n");
//	printf("\t-e \n");
	printf("\n");
//	printf("\t./sm3255_prog -e -c CID.bin -f ISP.bin -p Pretest.bin\n");
	printf("\t./sm3257En_prog -e EraseInfo -p PPret.bin -i IBCIDISP.bin \n");
	printf("\n");

	exit(1);
}


/*
 * Parse and validate user input
 */
#ifdef WIN32

int usb_init(){
	return true;
}

char *optarg;
static 	optind=0;
int getopt(int argc, char *argv[] , char *szParsing)
{
	int nRtnValue=-1;
	optarg=NULL;
	if(optind*2+1 < argc){
		if(strstr(argv[1+optind*2],"-")!= NULL){
			//it's command -e or -p or -i 
			char szTemp[3];
			sprintf(szTemp,"%c",argv[1+optind*2][1]);
			if(strstr(szParsing,szTemp) != NULL){
				nRtnValue = szTemp[0];
				optarg=argv[1+optind*2+1];
			}
		}
	}
	optind++;
	return nRtnValue;
}
#endif
int parse_cmdline_opts(int argc, char *argv[], int *cmd, int *mem_type, char *filename[])
{
	int i;
	int opt;
	char szoutput[128];
	OutputParam("Argc",argc);
//	while ((opt = getopt(argc, argv, "h:p:f:c:e:")) != -1) {
	while ((opt = getopt(argc, argv, "h:p:i:e:")) != -1) {
		switch(opt) {
		case 'h':
			return -1;		
		case 'p':
			OutputParams("PRETEST",optarg);
			filename[0] = optarg;
			//strcpy(filename[0] , optarg);	/* pretest filename */
			break;
		case 'i':
			OutputParams("ISP",optarg);
			filename[1] = optarg;
			//strcpy(filename[1] , optarg);	/* ISP filename */
			break;
//		case 'c':
//			OutputParams("CID",optarg);
//			filename[2] = optarg;
			//strcpy(filename[2] , optarg);	/* cid filename */
//			break;
		case 'e':
			OutputParams("EraseInfo",optarg);
			filename[3] = optarg;
			break;	
		default:
			return -1;
		}
	}
	for(i = 0 ; i <argc; i++){
		sprintf(szoutput,"i[%d] %d %d  %s\n",i,argc , optind,argv[i]);
		printf(szoutput);
	}
	return 0;
}

/*
 * To Get the strong Page table even not use the strong page download
*/
/*
BOOL DoUpdateStrongTable(	 pUSB_DEV_HANDLE udevh,	//mass storage driver handle
							 BYTE*	nStrongTable,	//The Strong Page Table Array
							 BOOL  bEnableStrong)	//The flag that enable/disable StrongPage Table
{
	int xx;
	
	int yy;
	BOOL success;
	UCHAR Command[16];
	int i,TotalMU,MUCount=0;
	SMI_DoReadFlashIDBuffer(udevh );
	memset(nStrongTable,0,STRONGPAGETABLESIZE);
	
	SM3257Enx_FCE = FlashIDBuf[0x130];//0xB0+0x80];	// bFirstCEPin
	SM3257Enx_FMU = FlashIDBuf[0x132];//0xB2+0x80];
	if(FlashIDBuf[0x00] & 0x01)
		SM3257Enx_TWIN = 2;
	else
		SM3257Enx_TWIN = 1;
	
	if(FlashIDBuf[0x01] & 0x40) 
		SM3257Enx_BlockSize = 256;	//BlockSize:256
	else if(FlashIDBuf[0x01] & 0x20)
		SM3257Enx_BlockSize = 128;	//BlockSize:128
	else
		SM3257Enx_BlockSize = 64;		//BlockSize:64
	
	if(FlashIDBuf[0x00] & 0x40) 
		SM3257Enx_PageSize = 8;	//PageSize:4K
	else if(FlashIDBuf[0x00] & 0x80)
		SM3257Enx_PageSize = 16;	//PageSize:8K
	else
		SM3257Enx_PageSize = 4;	//PageSize:2K
	TotalMU = FlashIDBuf[0xB5+0x80];	// bTotalFMU
	if(bEnableStrong)
	{
		for(xx = 1; xx < FlashIDBuf[0x16]; xx++)
		{
			for(yy = 0; yy < SM3257Enx_BlockSize; yy++)
			{
				if((TotalMU <= MUCount) || (MUCount > 64))
					break;
				
				memset(Command,0,sizeof(Command));
				Command[0x00] = 0xF0;
				Command[0x01] = 0x0A;
				Command[0x02] = SM3257Enx_FCE;
				Command[0x03] = 0;
				Command[0x04] = xx;
				Command[0x06] = yy;
				Command[0x09] = 0x04;
				Command[0x0A] = 0x00;
				Command[0x0B] = SM3257Enx_PageSize*SM3257Enx_TWIN;
				memset(SM3257Enx_DataBuffer,0,sizeof(SM3257Enx_DataBuffer));
				success=usb_scsi_read(udevh , SM3257Enx_DataBuffer,512*(SM3257Enx_PageSize*SM3257Enx_TWIN+1),Command,0x10);
				
				if((SM3257Enx_DataBuffer[512*(SM3257Enx_PageSize*SM3257Enx_TWIN)] == 0xE1))//  && (SM3257Enx_DataBuffer[512*(SM3257Enx_PageSize*SM3257Enx_TWIN)+1] != 0xFF)/)
				
					//=== Start Parsing the Page Table ===// ===>>>
					int nStrongIdx=0;
					for(i=0; i < STRONGPAGETABLESIZE ; i++){
						if( SM3257Enx_DataBuffer[0x19D+i/8]&(1<<(i%8)) ){
							nStrongTable[nStrongIdx]=i;
							nStrongIdx++;
						}
					}
					//=== Start Parsing the Page Table ===// <<<===
					MUCount=TotalMU;//For Breaking
				}
			}
		}
	}
	else
	{
		for(xx = 0 ; xx < STRONGPAGETABLESIZE ; xx++){
			nStrongTable[xx]=xx;
		}
	}
	return true;
}
*/

/*
 * Application entry point
 */
 /*
#ifdef Win32
int main(int argc, char *argv[])
{
	return SM3257Enmain(argc, argv);
}
#endif
*/

int main(int argc, char *argv[])
{
	int bSuccess;
//	int bEnableStrongTable=0;
	pUSB_DEVICE dev;
	pUSB_DEV_HANDLE udev;
	int cmd;
	int mem_type;
	char *filename[4]={"", "", "", ""};
	BYTE *write_buf,*write_bufA,*write_bufB;
	int nFileSize=0,nFileSizeA=0,nFileSizeB=0;
	OutputParams("Before","Parsing CmdLine");

	if (parse_cmdline_opts(argc, argv, &cmd, &mem_type, filename))
		usage();

	printf("--------------------\r\n");

	OutputParams("Before","usb_init");
	usb_init();
	OutputParams("Before","find_usb_dev");
	dev = find_usb_dev(SMI_VENDOR_ID, 0x1000);

	if (dev == NULL) {
		/* SM3257En device not found. Search for mem device instead. */
		dev = find_usb_dev(SMI_VENDOR_ID, 0x1000);

		if (dev == NULL) {
			printf("ERROR: SM3257En mem device not found\n");
			return -1;
		}

		printf("SM3257En mem device found\n\n");
	} else {
		printf("SM3257En device found\n\n");
	}

	udev = bulk_usb_open(dev);
	if (udev == NULL) {
		printf("ERROR: Couldn't open USB device!\n");
		return -1;
	}

	printf("-------------------->>>\n");
//0.Erase Info
	if(strlen(filename[3])){
		if(SMI_DoClearSystemBlocks(udev))
		{
			debug("Erase Info OK\n");
		}
		else
		{
			debug("Erase Info Fail\n");
		}
	}
	printf("-------------------->>>\n");

	//=== the production flow here ===// ===>>>
//1.Pretest
	if(strlen(filename[0])){
		OutputParams("PTest File",filename[0]);

		write_bufA=(BYTE*)malloc(MAX_READ_SIZE);
		write_bufB=(BYTE*)malloc(MAX_READ_SIZE);
		write_buf=(BYTE*)malloc(MAX_READ_SIZE);
	
		memset(write_bufA,0,MAX_READ_SIZE);
		memset(write_bufB,0,MAX_READ_SIZE);
		memset(write_buf,0,MAX_READ_SIZE);
	
		nFileSizeA = read_file("PPagePair.BIN",write_bufA);
		nFileSizeB = read_file("P512Para.BIN",write_bufB);
		nFileSize = read_file(filename[0],write_buf);
	

		if(nFileSizeA>0){
			bSuccess = SMI_DoPretestA(udev , write_bufA , nFileSizeA);
		}
		if(nFileSizeB>0){
			bSuccess = SMI_DoPretestB(udev , write_bufB , nFileSizeB);
		}
		if(nFileSize>0){
			bSuccess = SMI_DoPretest(udev , write_buf , nFileSize);
		}
		free(write_bufA);
		free(write_bufB);
		free(write_buf);
		
		SMI_DoReset(udev);
	    sleep(1);
	}
	printf("-------------------->>>\n");
	
//2.DoDownloadISP
	if(strlen(filename[1])){
		OutputParams("ISP File",filename[1]);

		write_bufA=(BYTE*)malloc(MAX_READ_SIZE);
		write_buf=(BYTE*)malloc(MAX_READ_SIZE);

		memset(write_bufA,0,MAX_READ_SIZE);
		memset(write_buf,0,MAX_READ_SIZE);
		
		nFileSizeA = read_file("IStrongPage.BIN",write_bufA);
		nFileSize = read_file(filename[1],write_buf);
		//OutputParam("nFileSize",nFileSize);
		
		if(SMI_DoDownloadISP(udev , write_buf , nFileSize , write_bufA)){
			debug("Download ISP OK\n");
		}
		else{
			debug("Download ISP Fail\n");
		}
		free(write_bufA);
		free(write_buf);

		SMI_DoReset(udev);
	    sleep(1);
	}

	printf("-------------------->>>\n");
	if (bulk_usb_close(udev) < 0) {
		printf("usb_close failed\n");
		return -1;
	}
	printf("Finished\r\n");
	return 0;
}

