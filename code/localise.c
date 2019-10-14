/*******************************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>

#define MessageFile				"message"
#define VALID_ARGUMENT_COUNT	(10)
#define VALID_BUOY_MAX 			(4)
#define TARGET_ARGV_NAME		"-t"
#define BUOY_ARGV_NAME			"-b"
#define CHECKSUM_ARGV_NAME		"-c"
#define ARGUMENT_RIGHT_FORMAT	"localise -t taggetId -b buoyId1 buoyId2 buoyId3 buoyId4 -c checksum\n"

#define TARGET_ARGV_NAME_INDEX	(1)
#define TARGET_ID_ARGV_INDEX	(2)
#define BUOY_ARGV_NAME_INDEX	(3)
#define BUOY_ID1_ARGV_INDEX		(4)
#define BUOY_ID2_ARGV_INDEX		(5)
#define BUOY_ID3_ARGV_INDEX		(6)
#define BUOY_ID4_ARGV_INDEX		(7)
#define CHECKSUM_ARGV_NAME_INDEX (8)
#define CHECKSUM_ARGV_INDEX		(9)

#define BP_MESSAGE_HEAD			"$BP,"
#define TR_MESSAGE_HEAD			"$TR,"

#define MESSAGE_HEAD_SIZE        4

struct bpMessage
{
	int times;
	int buoyId;
	int x;
	int y;
	int z;
	int checkSum;
	int range;

};

struct trMessage
{
	int times;
	int buoyId;
	int targetId;
	int range;
};

#define BP_MESSAGE_MAX		  100
#define TR_MESSAGE_MAX		  100
struct bpMessage bpMessageObj[BP_MESSAGE_MAX];
struct trMessage trMessageObj[TR_MESSAGE_MAX];

struct targetPos
{
	int x;
	int y;
	int z;
};
/*******************************************************************/
static int getBuoyPos(struct bpMessage *bp, int bpMessageCount,struct trMessage *tr, int trMessageCount,int *bpIndex,int *trIndex)
{
	int i = 0,j = 0;
	struct targetPos targetPosObj = {0,0,0};
	struct bpMessage *pBP = bp;
	int range[trMessageCount];
	int indexMin=-1; 
	for(i = 0; i < bpMessageCount; i++)
	{
		bp->range = pow(bp->x-targetPosObj.x,2)+pow(bp->y-targetPosObj.y,2)+pow(bp->z-targetPosObj.z,2);
		bp->range = sqrt(bp->range);
		pBP++;
		struct bpMessage *pTR = tr;
		for(j = 0; j < trMessageCount;j++)
		{
			range[j] = abs(pTR->range - bp->range);
			pTR++;
		}
	}
	//使用for循环判断
	int min=range[0]; //最大值
	for(i=0;i<trMessageCount;i++)
	{
		if(range[i]<min)
		{
			min=range[i];
			indexMin=i;
		}
	}
	*bpIndex = i;
	*trIndex = j;
	return 0;
}
/*******************************************************************/
static void stringSplit(char *src,const char *separator,char **dest,int *num)
 {
	/*
		src 源字符串的首地址(buf的地址) 
		separator 指定的分割字符
		dest 接收子字符串的数组
		num 分割后子字符串的个数
	*/
     char *pNext = NULL;
     int count = 0;
     if (src == NULL || strlen(src) == 0)//如果传入的地址为空或长度为0，直接终止 
	 {
        return;
	 }
     if (separator == NULL || strlen(separator) == 0)//如未指定分割的字符串，直接终止 
	 {
        return;
	 }
     pNext = (char *)strtok(src,separator);//必须使用(char*)进行强制类型转换(虽然不写有的编译器中不会出现指针错误)
     while(pNext != NULL) 
	 {
		*dest++ = pNext;
		++count;
		pNext = (char *)strtok(NULL,separator);//必须使用(char*)进行强制类型转换
    }  
    *num = count;
	return;
} 	
/*******************************************************************/
static int cheakSumFunction(char *messageText)
{
	if(NULL == messageText)
	{
		return -1;
	}
	int checksum = 0;
	char *p = messageText;
	while(*p != '\0')
	{
		checksum = checksum + *p;
		p++;
	}
	checksum = checksum & 0xFF;
	printf("the messageText checksum is %d\n",checksum);
	return checksum;
}
/*******************************************************************/
static int HasIdentical(const int* arr, int n)
{
	int i = 0, j = 0;
	for (i = 0; i < n; ++i)
	{
		for (j = i + 1; j < n; ++j)
		{
			if (arr[i] == arr[j])
			{
				return 1;
			}
		}
	}
	return 0;
}
/*******************************************************************/
//主函数
int main(int argc,char *argv[])
{
	int index = 0;
	int targetId = 0;
	int buoyId[VALID_BUOY_MAX] = {0};
	for(index = 0; index < argc; index++)
	{
		/*argv本身代表`char *argv[]`的第一个元素的地址，地址进行递增可以遍历数组，然后通过`*`来获取改地址对应的元素内容（这里元素的内容也是个地址*/
		printf("%s\n", argv[index]);
	}
	if(argc != VALID_ARGUMENT_COUNT)
	{
		printf("the argument count error!!!\n");
		printf("%s\n",ARGUMENT_RIGHT_FORMAT);
		return 0;
	}
	if(strcmp(TARGET_ARGV_NAME,argv[TARGET_ARGV_NAME_INDEX]))
	{
		printf("the target argumrnt type error!!!\n");
		printf("%s\n",ARGUMENT_RIGHT_FORMAT);
		return 2;
	}
	if(strcmp(BUOY_ARGV_NAME,argv[BUOY_ARGV_NAME_INDEX]))
	{
		printf("the buoy argumrnt type error!!!\n");
		printf("%s\n",ARGUMENT_RIGHT_FORMAT);
		return 2;
	}	
	if(strcmp(CHECKSUM_ARGV_NAME,argv[CHECKSUM_ARGV_NAME_INDEX]))
	{
		printf("the checksum argumrnt type error!!!\n");
		printf("%s\n",ARGUMENT_RIGHT_FORMAT);
		return 2;
	}	
	targetId = atoi(argv[TARGET_ID_ARGV_INDEX]);
	if((targetId < 1)||(targetId > 32767))
	{
		printf("the target Id is required!\n");
		return 0;
	}
	buoyId[0] = atoi(argv[BUOY_ID1_ARGV_INDEX]);
	buoyId[1] = atoi(argv[BUOY_ID2_ARGV_INDEX]);
	buoyId[2] = atoi(argv[BUOY_ID3_ARGV_INDEX]);
	buoyId[3] = atoi(argv[BUOY_ID4_ARGV_INDEX]);
	if(HasIdentical(buoyId,VALID_BUOY_MAX))
	{
		printf("the -b Id is repeated!\n");
		return 1;
	}
	if((buoyId[0]<0)||(buoyId[1]<0)||(buoyId[2]<0)||(buoyId[3]<0))
	{
		printf("less than 4 bouys is insufficient to localise a target.\n");
		return 3;
	}
	if(!(((buoyId[0]>targetId)&&(buoyId[1]>targetId)&&(buoyId[2]>targetId)&&(buoyId[3]>targetId))
		||((buoyId[0]<targetId)&&(buoyId[1]<targetId)&&(buoyId[2]<targetId)&&(buoyId[3]<targetId))))
	{
		printf("Target Id within buoy ID range.\n");
		return 4;
	}
	FILE *fp = NULL;
	char *lineMessage = NULL;
	unsigned long len = 0;
	size_t size = 0;
	int bpMessageCount=0,trMessageCount=0;
	char message[100]={0};
	char messageText[100]={0};
	char checkSumText[100]={0};
	fp = fopen(MessageFile,"r");//read message file 
	if(fp == NULL)
	{
		printf("the message file no exist!\n");
		return 0;
	}
	while(1) 
	{
		size = getline(&lineMessage,&len,fp);
		if (size == -1) 
		{
			break;
		}
		printf("the size is %ld, len is %ld,line is %s\n",size,len,lineMessage);
		strcpy(message,lineMessage);
		memset(messageText,0,sizeof(messageText));
		memset(checkSumText,0,sizeof(checkSumText));
		if(!strncmp(lineMessage,BP_MESSAGE_HEAD,MESSAGE_HEAD_SIZE))//浮标坐标消息类型
		{
			char *p=&message[1];
			int flag = 0;
			int i = 0;
			while(*p != '\0')
			{
				printf("%c",*p);
				if(*p == '*')
				{
					flag = 1;
					p++;
					i = 0;
					continue;
				}
				if(0==flag)
				{
					messageText[i] = *p;
				}
				else
				{
					checkSumText[i] = *p;
				}
				p++;
				i++;
			}
			printf("the messageText is %s, the checkSumText is %s\n",messageText,checkSumText);
			int checkSum1 = 0, checkSum2 = 0;
			checkSum1=cheakSumFunction(messageText);
			checkSum2=atoi(checkSumText);
			if(checkSum1==checkSum2)
			{
				printf("the bp message is valid!!!\n");
				char *revbuf[6] = {NULL};//存放分割后的子字符串 
				//分割后子字符串的个数
				int num = 0;
				stringSplit(messageText,",",revbuf,&num);//调用函数进行分割 
				//输出返回的每个内容 
				for(i = 0;i < num; i ++)
				{
					printf("%s\n",revbuf[i]);
				}
				if(bpMessageCount < BP_MESSAGE_MAX)
				{
					
					bpMessageObj[bpMessageCount].times = atoi(revbuf[1]);
					bpMessageObj[bpMessageCount].buoyId = atoi(revbuf[2]);
					bpMessageObj[bpMessageCount].x  = atoi(revbuf[3]);
					bpMessageObj[bpMessageCount].y  = atoi(revbuf[4]);
					bpMessageObj[bpMessageCount].z  = atoi(revbuf[5]);
					bpMessageObj[bpMessageCount].checkSum = checkSum1;
					bpMessageCount++;
				}
			}
			else
			{
				printf("Error:checksum for message %s failed\n",messageText);
			}
		}
		else if(!strncmp(lineMessage,TR_MESSAGE_HEAD,MESSAGE_HEAD_SIZE))//目标范围消息类型
		{
			char *p=&message[1];
			int flag = 0;
			int i = 0;
			while(*p != '\0')
			{
				printf("%c",*p);
				if(*p == '*')
				{
					flag = 1;
					p++;
					i = 0;
					continue;
				}
				if(0==flag)
				{
					messageText[i] = *p;
				}
				else
				{
					checkSumText[i] = *p;
				}
				p++;
				i++;
			}
			printf("the messageText is %s, the checkSumText is %s\n",messageText,checkSumText);
			int checkSum1 = 0, checkSum2 = 0;
			checkSum1=cheakSumFunction(messageText);
			checkSum2=atoi(checkSumText);
			if(checkSum1==checkSum2)
			{
				printf("the tr message is valid!!!\n");
				char *revbuf[6] = {NULL};//存放分割后的子字符串 
				//分割后子字符串的个数
				int num = 0;
				stringSplit(messageText,",",revbuf,&num);//调用函数进行分割 
				//输出返回的每个内容 
				for(i = 0;i < num; i ++)
				{
					printf("%s\n",revbuf[i]);
				}
				if(trMessageCount < TR_MESSAGE_MAX)
				{
					
					trMessageObj[trMessageCount].times = atoi(revbuf[1]);
					trMessageObj[trMessageCount].buoyId = atoi(revbuf[2]);
					trMessageObj[trMessageCount].targetId  = atoi(revbuf[3]);
					trMessageObj[trMessageCount].range = atoi(revbuf[4]);
					trMessageCount++;
				}
			}
			else
			{
				printf("Error:checksum for message %s failed\n",messageText);
			}
		}
		else
		{
			printf("message type error,the message will ignored!\n");
		}
	}
	int i = 0, j = 0;
	getBuoyPos(bpMessageObj,bpMessageCount,trMessageObj,trMessageCount,&i,&j);
	printf("$TP %d %d %d %d %d*%d\n",trMessageObj[j].times,trMessageObj[j].targetId,bpMessageObj[i].x,bpMessageObj[i].y,bpMessageObj[i].z,bpMessageObj[i].checkSum);
	free(lineMessage);
	fclose(fp);
	return 0;
}
/*******************************************************************/

