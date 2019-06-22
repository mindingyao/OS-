#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

#define BLOCK_SIZE 512
#define BLOCK_COUNT 512
#define INODE_COUNT 512

#define FILENAME "file.txt"


typedef struct 
{
	unsigned int block_size;  //块大小
	unsigned int block_count; //块数量
	unsigned int inode_count; //节点数量
	unsigned int free_block_count; //空闲的块数量
	unsigned int free_inode_count; //空闲的节点数量
}SuperBlock;

typedef struct 
{
	unsigned int i_num; //编号
	char filename[20]; //文件名
	unsigned short i_mode;  //the format of the described file and the access rights
							//0:file 1:dic
	unsigned char type; //0: read only 1: read-write
        time_t t;//文件最后修改时间
	unsigned int i_size; //大小
	unsigned int i_blocks; //占多少个块，每个块512字节
	unsigned int parent; //上级目录

	//unsigned int i_links_count; //链接数
	unsigned int i_addr1[12]; //文件前10项为直接索引，目录前12项为直接索引
}Inode;

typedef struct{
    char userName[20];
	char passWord[20];
}User;

typedef struct 
{
	unsigned int i_num;//文件占用的第一个block块号
	char filename[20];
	unsigned int i_mode;
}FCB;

int H = BLOCK_SIZE/sizeof(FCB); //每个block能存下几个FCB


char BlockBitmap[BLOCK_COUNT];
char InodeBitmap[INODE_COUNT];

SuperBlock superBlock;
User curUser = (User){"min", "1915552319"};

unsigned int currentDir; //当前目录
FILE *fp, *pFile;
Inode *filenode;
char argv[5][20];
//char cmd[5][20];命令行输入

void create()
{
	long len;
	printf("系统正在初始化……\n");

	fp = fopen(FILENAME, "wb+");
	if (fp == NULL)
	{
		printf("创建文件错误，请重新创建\n");
		exit(1);
	}

	//初始化位图
	for (len=0; len<BLOCK_COUNT; len++)
	{
		BlockBitmap[len] = 0;
	}

	for (len=0; len<INODE_COUNT; len++)
	{
		InodeBitmap[len] = 0;
	}

	//分配磁盘空间，即把磁盘空间全被写入0
	long SIZE = sizeof(superBlock)+sizeof(BlockBitmap)
				+sizeof(InodeBitmap)+sizeof(Inode)*INODE_COUNT+sizeof(BLOCK_SIZE)*BLOCK_COUNT;
	for(len=0; len<SIZE; len++)
	{
		fputc(0, fp);
	}

	//将文件指针移到文件头部
	rewind(fp);

	//初始化superblock
	superBlock.block_size = BLOCK_SIZE;
	superBlock.block_count = BLOCK_COUNT;
	superBlock.inode_count = INODE_COUNT;
	superBlock.free_block_count = BLOCK_COUNT;
	superBlock.free_inode_count = INODE_COUNT-1;

	fwrite(&superBlock, sizeof(superBlock), 1, fp);

	//创建根目录
	Inode *iroot = (Inode *)malloc(sizeof(Inode));
	iroot->i_num = 0;
	strcpy(iroot->filename , "/");
	iroot->i_mode = 1; //目录文件
	iroot->i_blocks = 0;
	iroot->i_size = 0; //目录下文件数为0
	iroot->parent = 0;

	//iroot->i_addr1[0] = 0;


	InodeBitmap[0] = 1;
	//BlockBitmap[0] = 1;
	fseek(fp, sizeof(superBlock), SEEK_SET);
	fwrite(BlockBitmap, sizeof(BlockBitmap), 1, fp);
	fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap), SEEK_SET);
	fwrite(InodeBitmap, sizeof(InodeBitmap), 1, fp);
	fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				INODE_COUNT*sizeof(Inode), SEEK_SET);
	fwrite(iroot, sizeof(Inode), 1, fp);
	fflush(fp);

	currentDir = 0;
	//write a log
	pFile = fopen("log.txt", "a");
    time_t time_log = time(NULL);
	struct tm* tm_log = localtime(&time_log);
	fprintf(pFile, "%04d-%02d-%02d %02d:%02d:%02d has created a file system\n", tm_log->tm_year + 1900, tm_log->tm_mon + 1, tm_log->tm_mday, tm_log->tm_hour, tm_log->tm_min, tm_log->tm_sec);

	fflush(pFile);
	fclose(pFile);

}

void openFileSystem()
{
        if ((fp = fopen(FILENAME, "rb+")) == NULL)
		create();
	else
	{
		rewind(fp);
		fread(&superBlock, sizeof(SuperBlock), 1, fp);
		fread(BlockBitmap, sizeof(BlockBitmap), 1, fp);
		fread(InodeBitmap, sizeof(InodeBitmap), 1, fp);

		currentDir = 0;
	}
        pFile = fopen("log.txt", "a");
        time_t time_log = time(NULL);
	struct tm* tm_log = localtime(&time_log);
	fprintf(pFile, "%04d-%02d-%02d %02d:%02d:%02d has opened the file system\n", tm_log->tm_year + 1900, tm_log->tm_mon + 1, tm_log->tm_mday, tm_log->tm_hour, tm_log->tm_min, tm_log->tm_sec);

	fflush(pFile);
	fclose(pFile);
}

int findInodeNum(char *name, int mode);

void create_file(char *name, int mode)
{
	if(name == NULL || strcmp(name, "") == 0 || findInodeNum(name, mode) != -1)
    {
        printf("invalid file name:the name is empty,or the file has existed\n");
        return ;
    }

  int curBlock, curInode, i, j;
	Inode *fileInode = (Inode *)malloc(sizeof(Inode));
	Inode *parentInode = (Inode *)malloc(sizeof(Inode));
	FCB *fcb = (FCB *)malloc(sizeof(FCB));

	

	//寻找空闲inode
	for(i=0; i<INODE_COUNT; i++)
	{
		if (InodeBitmap[i] == 0)
		{
			curInode = i;
			InodeBitmap[i] = 1;
			break;
		}
	}

	fileInode->i_blocks = 0;
	//fileInode->i_addr1[0] = curBlock;
	strcpy(fileInode->filename, name);
	fileInode->i_num = curInode;
	fileInode->parent = currentDir;
	fileInode->i_mode = mode;
	fileInode->type = '1';
        time(&(fileInode->t));
	fileInode->i_size = 0;

	fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
					sizeof(Inode)*curInode,SEEK_SET);
	fwrite(fileInode, sizeof(Inode), 1, fp);

	//更新superblock的信息
    rewind(fp);
	fread(&superBlock, sizeof(SuperBlock), 1, fp);
	fread(BlockBitmap, sizeof(BlockBitmap), 1, fp);
	fread(InodeBitmap, sizeof(InodeBitmap), 1, fp);
	//superBlock.free_block_count -= 1;
	superBlock.free_inode_count -= 1;
	//BlockBitmap[curBlock] = 1;
	InodeBitmap[curInode] = 1;

	strcpy(fcb->filename, fileInode->filename);
	fcb->i_num = fileInode->i_num;
	fcb->i_mode = fileInode->i_mode;

	//更新上级目录的信息
	//这里可以改进为判断是否目录信息超过了一个block，可以继续寻找空闲block
	fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
					sizeof(Inode)*currentDir,SEEK_SET);
	fread(parentInode, sizeof(Inode), 1, fp);
	for (i=0; i<11; i++)
		if (parentInode->i_size == i*H)
		{
			//寻找空闲物理块，目前先采用最低效的遍历，后续再考虑其他分配算法
		  parentInode->i_blocks = parentInode->i_blocks+1;
			for	(j=0; j<BLOCK_COUNT; j++)
			{
				if (BlockBitmap[j] == 0)
				{
					curBlock = j;
					BlockBitmap[j] = 1;
					parentInode->i_addr1[i] = curBlock;
					superBlock.free_block_count -= 1;
					break;
				}
			}
		}
	fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
			sizeof(Inode)*INODE_COUNT+BLOCK_SIZE*parentInode->i_addr1[parentInode->i_size/H]+
			(parentInode->i_size % H)*sizeof(FCB),SEEK_SET);
	fwrite(fcb, sizeof(FCB), 1, fp);
	parentInode->i_size += 1;
	fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
					sizeof(Inode)*currentDir,SEEK_SET);
	fwrite(parentInode, sizeof(Inode), 1, fp);

	rewind(fp);
	fwrite(&superBlock, sizeof(SuperBlock), 1, fp);
	fwrite(BlockBitmap, sizeof(BlockBitmap), 1, fp);
	fwrite(InodeBitmap, sizeof(InodeBitmap), 1, fp);

	free(fileInode);
	free(parentInode);
	free(fcb);
}

int findInodeNum(char *name, int mode)
{
  int fileInodeNum, i, j;
	Inode *parentInode = (Inode *)malloc(sizeof(Inode));
	FCB *fcb = (FCB *)malloc(sizeof(FCB));

	fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				currentDir*sizeof(Inode), SEEK_SET);
	fread(parentInode, sizeof(Inode), 1, fp);

	for (i=0; i<=parentInode->i_size/H; i++)
	{
		for (j=0; j<H && (i*H+j)<parentInode->i_size; j++)
		{
		  fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
			INODE_COUNT*sizeof(Inode)+parentInode->i_addr1[i]*BLOCK_SIZE+j*sizeof(FCB), SEEK_SET);
		  fread(fcb, sizeof(FCB), 1, fp);
		        
			if (mode == 0)
			{
				if (fcb->i_mode == 0 && strcmp(name, fcb->filename)==0)
				{
					fileInodeNum = fcb->i_num;
					free(fcb);
                                 	free(parentInode);
	                                return fileInodeNum;
				}
			}
			else
			{
				if (fcb->i_mode == 1 && strcmp(name, fcb->filename)==0)
				{
					fileInodeNum = fcb->i_num;
					free(fcb);
                                 	free(parentInode);
	                                return fileInodeNum;
				}
			}
		}
	}

        
	return -1;
}

//内容写入文件
void write(char *name)
{
	char text[512*512],ch;
	int fileInodeNum, len=0, i, j, h;
	int single[BLOCK_SIZE/32];
	int double1[BLOCK_SIZE/32];
	int double2[BLOCK_SIZE/32][BLOCK_SIZE/32];
	Inode *fileInode = (Inode *)malloc(sizeof(Inode));
	if((fileInodeNum=findInodeNum(name,1))!=-1)
	{
		printf("This is a directory,not a file...\n");
		return;
	}
	fileInodeNum=findInodeNum(name,0);
	if(fileInodeNum==-1)
		printf("This is no %s file...\n",name);
	else
	{
		fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				fileInodeNum*sizeof(Inode), SEEK_SET);
		fread(fileInode, sizeof(Inode), 1, fp);
		if (fileInode->type == '0')
		{
			printf("file %s is Read-Only file\n", name);
			return;
		}
		printf("please input file content(stop by #):\n");
		while((ch=getchar())!='#')
		{
			text[len]=ch;
			len++;
		}
		char *Text = (char *)malloc(sizeof(char)*len);
		for (j=0; j<len; j++)
		{
			Text[j] = text[j];
		}
	        rewind(fp);
         	fread(&superBlock, sizeof(SuperBlock), 1, fp);
	        fread(BlockBitmap, sizeof(BlockBitmap), 1, fp);
		fileInode->i_size = len;
		fileInode->i_blocks = fileInode->i_size/512 + 1;
		int c = fileInode->i_blocks;
		printf("\nlen: %d blcoks: %d\n", len, c);
		int b[512];
		int flag = 0;
		//b[flag++] = fileInode->i_addr1[0];
		for (i=0; i<BLOCK_COUNT; i++)
		{
			if (BlockBitmap[i] == 0)
			{
				b[flag++] = i;
				//printf("%d\n", b[flag-1]);
				BlockBitmap[i] = 1;
			}
			if (flag>=c)
			{
				break;
			}
		}
		for (i=0; i<c; i++)
		{
			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
					INODE_COUNT*sizeof(Inode)+BLOCK_COUNT*b[i], SEEK_SET);
			for(j=0; j<BLOCK_SIZE && (i*BLOCK_SIZE+j)<fileInode->i_size; j++)
			{
				fputc(Text[i*BLOCK_SIZE+j], fp);
			}
		}

		pFile = fopen("log.txt", "a");
		time_t time_log = time(NULL);
		struct tm* tm_log = localtime(&time_log);
		fprintf(pFile, "%04d-%02d-%02d %02d:%02d:%02d has written %s\n", tm_log->tm_year + 1900, tm_log->tm_mon + 1, tm_log->tm_mday, tm_log->tm_hour, tm_log->tm_min, tm_log->tm_sec, name);

		fflush(pFile);
		fclose(pFile);
		//更新节点
		fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						fileInodeNum*sizeof(Inode), SEEK_SET);
		fread(fileInode, sizeof(Inode), 1, fp);
		fileInode->i_size = len;
		fileInode->i_blocks = c;
		if(c<=10)
		{
			for (i=0; i<c; i++)
			{
				fileInode->i_addr1[i] = b[i];
			}
		}
		else if (c >10 && c<=(10+BLOCK_SIZE/32)) //一级索引
		{
			for (i=0; i<10; i++)
			{
				fileInode->i_addr1[i] = b[i];
			}
			for (i=0; i<BLOCK_COUNT; i++)
			{
				if (BlockBitmap[i] == 0)
				{
					fileInode->i_addr1[10] = i;
					BlockBitmap[i] = 1;
					break;
				}
			}
			for (i=10; i<c; i++)
			{
				single[i-10] = b[i];
			}
			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						INODE_COUNT*sizeof(Inode)+fileInode->i_addr1[10]*BLOCK_SIZE, SEEK_SET);
			fwrite(single, BLOCK_SIZE, 1, fp);
			superBlock.free_block_count -= 1;

		}
		else
		{
			for (i=0; i<10; i++)
			{
				fileInode->i_addr1[i] = b[i];
			}
			for (i=10; i<(10+BLOCK_SIZE/32); i++)
			{
				single[i-10] = b[i];
			}
			for (i=0; i<BLOCK_COUNT; i++)
			{
				if (BlockBitmap[i] == 0)
				{
					fileInode->i_addr1[10] = i;
					BlockBitmap[i] = 1;
					break;
				}
			}
			for (i=0; i<BLOCK_COUNT; i++)
			{
				if (BlockBitmap[i] == 0)
				{
					fileInode->i_addr1[11] = i;
					BlockBitmap[i] = 1;
					break;
				}
			}
			h = (c-(10+BLOCK_SIZE/32))/(BLOCK_SIZE/32)+1;
			for (i=0, j=0; i<BLOCK_COUNT; i++)
			{
				if (BlockBitmap[i] == 0)
				{
					double1[j++] = i;
					BlockBitmap[i] = 1;
				}
				if (j>=h)
					break;
			}
		
			for (i=0; i<h; i++)
			{
				for (j=0; j<BLOCK_SIZE/32; j++)
				{
					double2[i][j] = b[(10+BLOCK_SIZE/32)+i*BLOCK_SIZE/32+j];
					if ((10+BLOCK_SIZE/32+i*BLOCK_SIZE/32+j+1)>=c)
						break;
				}
				if ((10+BLOCK_SIZE/32+i*BLOCK_SIZE/32+j+1)>=c)
						break;
			}
			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						INODE_COUNT*sizeof(Inode)+fileInode->i_addr1[10]*BLOCK_SIZE, SEEK_SET);
			fwrite(single, BLOCK_SIZE, 1, fp);
			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						INODE_COUNT*sizeof(Inode)+fileInode->i_addr1[11]*BLOCK_SIZE, SEEK_SET);
			fwrite(double1, BLOCK_SIZE, 1, fp);
			for (i=0; i<h; i++)
			{
				fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						INODE_COUNT*sizeof(Inode)+double1[i]*BLOCK_SIZE, SEEK_SET);
				fwrite(double2[i], BLOCK_SIZE, 1, fp);
			}
			superBlock.free_block_count -= (1+1+h);
		}
		superBlock.free_block_count -= c;
	        time(&(fileInode->t));
		fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						fileInodeNum*sizeof(Inode), SEEK_SET);
		fwrite(fileInode, sizeof(Inode), 1, fp);
		rewind(fp);
		fwrite(&superBlock, sizeof(SuperBlock), 1, fp);
		fwrite(BlockBitmap, sizeof(BlockBitmap), 1, fp);
	}

	pFile = fopen("log.txt", "a");
        time_t time_log = time(NULL);
	struct tm* tm_log = localtime(&time_log);
	fprintf(pFile, "%04d-%02d-%02d %02d:%02d:%02d has updated the filenode!\n", tm_log->tm_year + 1900, tm_log->tm_mon + 1, tm_log->tm_mday, tm_log->tm_hour, tm_log->tm_min, tm_log->tm_sec);

	fflush(pFile);
	fclose(pFile);
	free(fileInode);
}

void read(char *name)
{
        int fileInodeNum,i,j,h=0;
	char c;
	int len, nums; //长度和占用物理块数
	int single[BLOCK_SIZE/32];
	int double1[BLOCK_SIZE/32];
	int double2[BLOCK_SIZE/32][BLOCK_SIZE/32];
	Inode *fileInode = (Inode *)malloc(sizeof(Inode));
	if((fileInodeNum=findInodeNum(name,1))!=-1)
	{
		printf("This is a directory,not a file...\n");
		return;
	}
	fileInodeNum=findInodeNum(name,0);
	if(fileInodeNum==-1)
		printf("This is no %s file...\n",name);
	else
	{
		fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
					fileInodeNum*sizeof(Inode), SEEK_SET);
		fread(fileInode, sizeof(Inode), 1, fp);
		len = fileInode->i_size;
		nums = fileInode->i_blocks;
	        if (nums<=10)
		{	
			int l=0;
			for (i=0; i<nums; i++)
			{
				fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
							+INODE_COUNT*sizeof(Inode)+fileInode->i_addr1[i]*BLOCK_SIZE,SEEK_SET);
				while(l<BLOCK_SIZE*(i+1) && l<len)
				{
				  
					c = fgetc(fp);
					putchar(c);
					l++;
				}
			}
			//printf("\nread len: %d\n", l);
		}
		else if (nums >10 && nums<=(10+BLOCK_SIZE/32))
		{
			int l=0;
			for (i=0; i<10; i++)
			{
				fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
							+INODE_COUNT*sizeof(Inode)+fileInode->i_addr1[i]*BLOCK_SIZE,SEEK_SET);
				while(l<BLOCK_SIZE*(i+1) && l<len)
				{
				  
					c = fgetc(fp);
					putchar(c);
					l++;
				}
			}
			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
							+INODE_COUNT*sizeof(Inode)+fileInode->i_addr1[10]*BLOCK_SIZE,SEEK_SET);
			fread(single, BLOCK_SIZE, 1, fp);
			for (i=10; i<nums; i++)
			{
				fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
							+INODE_COUNT*sizeof(Inode)+single[i-10]*BLOCK_SIZE,SEEK_SET);
				while(l<BLOCK_SIZE*(i+1) && l<len)
				{ 
					c = fgetc(fp);
					putchar(c);
					l++;
				}
			}
			//printf("\nread len: %d\n", l);
		}
		else
		{
			h = (nums-(10+BLOCK_SIZE/32))/(BLOCK_SIZE/32)+1;
			int l=0;
			for (i=0; i<10; i++)
			{
				fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
							+INODE_COUNT*sizeof(Inode)+fileInode->i_addr1[i]*BLOCK_SIZE,SEEK_SET);
				while(l<BLOCK_SIZE*(i+1) && l<len)
				{
				  
					c = fgetc(fp);
					putchar(c);
					l++;
				}
			}
			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
							+INODE_COUNT*sizeof(Inode)+fileInode->i_addr1[10]*BLOCK_SIZE,SEEK_SET);
			fread(single, BLOCK_SIZE, 1, fp);
			for (i=10; i<(10+BLOCK_SIZE/32); i++)
			{
				fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
							+INODE_COUNT*sizeof(Inode)+single[i-10]*BLOCK_SIZE,SEEK_SET);
				while(l<BLOCK_SIZE*(i+1) && l<len)
				{ 
					c = fgetc(fp);
					putchar(c);
					l++;
				}
			}
			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
							+INODE_COUNT*sizeof(Inode)+fileInode->i_addr1[11]*BLOCK_SIZE,SEEK_SET);
			fread(double1, BLOCK_SIZE, 1, fp);
			for (i=0; i<h; i++)
			{
				fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
							+INODE_COUNT*sizeof(Inode)+double1[i]*BLOCK_SIZE,SEEK_SET);
				fread(double2[i], BLOCK_SIZE, 1, fp);
				for (j=0; j<BLOCK_SIZE/32; j++)
				{
					fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
							+INODE_COUNT*sizeof(Inode)+double2[i][j]*BLOCK_SIZE,SEEK_SET);
					while (l<len && l<BLOCK_SIZE*(10+BLOCK_SIZE/32+i*BLOCK_SIZE/32+j+1))
					{
						c = fgetc(fp);
						putchar(c);
						l++;
					}
				}
			}
		}
	}
	printf("\n");
	pFile = fopen("log.txt", "a");
    time_t time_log = time(NULL);
	struct tm* tm_log = localtime(&time_log);
	fprintf(pFile, "%04d-%02d-%02d %02d:%02d:%02d has read a file named %s\n", tm_log->tm_year + 1900, tm_log->tm_mon + 1, tm_log->tm_mday, tm_log->tm_hour, tm_log->tm_min, tm_log->tm_sec, name);

	fflush(pFile);
	fclose(pFile);
	free(fileInode);
}

void cd(char *name);

void delete1(char *name)
{
  int fileInodeNum, i, j, h, k, l, m, t;
	int single[BLOCK_SIZE/32];
	int double1[BLOCK_SIZE/32];
	int double2[BLOCK_SIZE/32][BLOCK_SIZE/32];
	Inode *fileInode = (Inode *)malloc(sizeof(Inode));
	Inode *parentInode = (Inode *)malloc(sizeof(Inode));
	Inode *curInode = (Inode *)malloc(sizeof(Inode));
	FCB fcb[100];
	//printf("%d\n", currentDir);
	if(((fileInodeNum=findInodeNum(name,0))==-1)&&((fileInodeNum=findInodeNum(name,1))==-1))
	{
		printf("This is no %s...\n",name);
	}

	else
	{
		if ((findInodeNum(name,0)) != -1)
		{

			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						fileInodeNum*sizeof(Inode), SEEK_SET);
			fread(fileInode, sizeof(Inode), 1, fp);

			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						fileInode->parent*sizeof(Inode), SEEK_SET);
			fread(parentInode, sizeof(Inode), 1, fp);

			for (i=0; i<=parentInode->i_size/H; i++)
			{
				fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
							INODE_COUNT*sizeof(Inode)+parentInode->i_addr1[i]*BLOCK_SIZE, SEEK_SET);
				for (j=0; j<H && (i*H+j)<parentInode->i_size; j++)
				{
					fread(&fcb[i*H+j], sizeof(FCB), 1, fp);
				}
			}

			t = 0;
			for (i=0; i<=parentInode->i_size/H;i++)
			{
				fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
							INODE_COUNT*sizeof(Inode)+parentInode->i_addr1[i]*BLOCK_SIZE, SEEK_SET);
							
				for (j=0; j<H && (i*H+j)<parentInode->i_size-1; )
				{
					if((strcmp(fcb[t].filename,name))!=0)
					{
						fwrite(&fcb[t],sizeof(FCB),1,fp);
						j++;
					}
					t++;
				}
			}
		        i = i-1;
			rewind(fp);
			fread(&superBlock, sizeof(SuperBlock), 1, fp);
			fread(BlockBitmap, sizeof(BlockBitmap), 1, fp);
			fread(InodeBitmap, sizeof(InodeBitmap), 1, fp);
			if (parentInode->i_size == i*H+1)
			{
				parentInode->i_blocks = i;
				BlockBitmap[parentInode->i_addr1[i]] = 0;
				superBlock.free_block_count+=1;
				//printf("++2\n");
				}
			parentInode->i_size -= 1;
			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						fileInode->parent*sizeof(Inode), SEEK_SET);
			fwrite(parentInode, sizeof(Inode), 1, fp);
			InodeBitmap[fileInodeNum] = 0;
			int nums = fileInode->i_blocks;
			if (nums <= 10)
			{
				for (i=0; i<nums; i++)
				{
					BlockBitmap[fileInode->i_addr1[i]] = 0;
				}
			}
			else if (nums >10 && nums<=(10+BLOCK_SIZE/32))
			{
				for (i=0; i<10; i++)
				{
					BlockBitmap[fileInode->i_addr1[i]] = 0;
				}
				fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						fileInode->i_addr1[10]*sizeof(Inode), SEEK_SET);
				fread(single, BLOCK_SIZE, 1, fp);
				for (i=10; i<nums; i++)
				{
					BlockBitmap[single[i-10]] = 0;
				}
				BlockBitmap[fileInode->i_addr1[10]] = 0;
				superBlock.free_block_count += 1;
			}
			else
			{
				h = (nums-(10+BLOCK_SIZE/32))/(BLOCK_SIZE/32)+1;
				for (i=0; i<10; i++)
				{
					BlockBitmap[fileInode->i_addr1[i]] = 0;
				}
				fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						fileInode->i_addr1[10]*sizeof(Inode), SEEK_SET);
				fread(single, BLOCK_SIZE, 1, fp);
				for (i=10; i<(10+BLOCK_SIZE/32); i++)
				{
					BlockBitmap[single[i-10]] = 0;
				}
				BlockBitmap[fileInode->i_addr1[10]] = 0;
				fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						INODE_COUNT*sizeof(Inode)+fileInode->i_addr1[11]*BLOCK_SIZE, SEEK_SET);
				fread(double1, BLOCK_SIZE, 1, fp);
				for (i=0; i<h; i++)
				{
					fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						INODE_COUNT*sizeof(Inode)+double1[i]*BLOCK_SIZE, SEEK_SET);
					fread(double2[i], BLOCK_SIZE, 1, fp);
					for (j=0; j<BLOCK_SIZE/4; j++)
					{
						BlockBitmap[double2[i][j]] = 0;
						if ((10+BLOCK_SIZE/32+i*BLOCK_SIZE/32+j+1)>=nums)
							break;
					}
					if ((10+BLOCK_SIZE/32+i*BLOCK_SIZE/32+j+1)>=nums)
							break;
				}
				BlockBitmap[fileInode->i_addr1[11]] = 0;
				superBlock.free_block_count += (1+1+h);
			}
			superBlock.free_block_count += nums;
			superBlock.free_inode_count += 1;
			rewind(fp);
			fwrite(&superBlock, sizeof(SuperBlock), 1, fp);
			fwrite(BlockBitmap, sizeof(BlockBitmap), 1, fp);
			fwrite(InodeBitmap, sizeof(InodeBitmap), 1, fp);
		}
		else
		{
			//printf("this is not a regular file\n");
			//删除子文件
			cd(name);
			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						currentDir*sizeof(Inode), SEEK_SET);
			fread(curInode, sizeof(Inode), 1, fp);
			for (i=0; i<=curInode->i_size/H; i++)
			{
				for (j=0; j<H && (i*H+j)<curInode->i_size; j++)
				{
					fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
					      INODE_COUNT*sizeof(Inode)+curInode->i_addr1[i]*BLOCK_SIZE, SEEK_SET);
					fread(&fcb[j], sizeof(FCB), 1, fp);
					//printf("%s\n", fcb[j].filename);
					delete1(fcb[j].filename);
				}
			}
			cd("..");
			//删除目录文件本身
			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						fileInodeNum*sizeof(Inode), SEEK_SET);
			fread(fileInode, sizeof(Inode), 1, fp);

			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						fileInode->parent*sizeof(Inode), SEEK_SET);
			fread(parentInode, sizeof(Inode), 1, fp);

			for (k=0; k<=parentInode->i_size/H; k++)
			{
				fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
							INODE_COUNT*sizeof(Inode)+parentInode->i_addr1[k]*BLOCK_SIZE, SEEK_SET);
				for (l=0; l<H && (k*H+l)<parentInode->i_size; l++)
				{
					fread(&fcb[k*H+l], sizeof(FCB), 1, fp);
				}
			}
			t = 0;
			for (k = 0; k<=parentInode->i_size/H; k++)
			{
				fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
							INODE_COUNT*sizeof(Inode)+parentInode->i_addr1[k]*BLOCK_SIZE, SEEK_SET);
							
				for (l=0; l<H && (k*H+l)<parentInode->i_size-1;)
				{
					if((strcmp(fcb[t].filename,name))!=0)
					{
					  printf("%s\n", fcb[t].filename);
							fwrite(&fcb[t],sizeof(FCB),1,fp);
							l++;
					}
					t++;
				}
				}
			k = k-1;
			rewind(fp);
			fread(&superBlock, sizeof(SuperBlock), 1, fp);
			fread(BlockBitmap, sizeof(BlockBitmap), 1, fp);
			fread(InodeBitmap, sizeof(InodeBitmap), 1, fp);
			if (parentInode->i_size == k*H+1)
			{
				parentInode->i_blocks = k;
				BlockBitmap[parentInode->i_addr1[k]] = 0;
				superBlock.free_block_count+=1;
			}
			parentInode->i_size -= 1;
		        time(&(parentInode->t));
			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						fileInode->parent*sizeof(Inode), SEEK_SET);
			fwrite(parentInode, sizeof(Inode), 1, fp);
		   
			superBlock.free_inode_count += 1;
			InodeBitmap[fileInode->i_num] = 0;
			rewind(fp);
			fwrite(&superBlock, sizeof(SuperBlock), 1, fp);
			fwrite(BlockBitmap, sizeof(BlockBitmap), 1, fp);
			fwrite(InodeBitmap, sizeof(InodeBitmap), 1, fp);
			pFile = fopen("log.txt", "a");
			time_t time_log = time(NULL);
			struct tm* tm_log = localtime(&time_log);
			fprintf(pFile, "%04d-%02d-%02d %02d:%02d:%02d has deleted a file named %s\n", tm_log->tm_year + 1900, tm_log->tm_mon + 1, tm_log->tm_mday, tm_log->tm_hour, tm_log->tm_min, tm_log->tm_sec, name);

			fflush(pFile);
			fclose(pFile);
		}

	}

	free(fileInode);
	free(parentInode);

}


void cd(char *name)
{
	int fileInodeNum;
	Inode *fileInode = (Inode *)malloc(sizeof(Inode));

	if (strcmp(name, "..") == 0)
	{
	        fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				currentDir*sizeof(Inode), SEEK_SET);
		fread(fileInode, sizeof(Inode), 1, fp);
		currentDir = fileInode->parent;
	}
	else if(strcmp(name, ".")==0)
	  return;
	else if (strcmp(name, " ") == 0)
	  currentDir = 0;
	else
	{
		fileInodeNum = findInodeNum(name, 1);
		if (fileInodeNum == -1)
			printf("this is no %s directory\n", name);
		else
			currentDir = fileInodeNum;
	}

	free(fileInode);
}




void ls_l()
{
	int i, j;
	FCB *fcb = (FCB *)malloc(sizeof(FCB));
	Inode *fileInode = (Inode *)malloc(sizeof(Inode));
	Inode *parentInode = (Inode *)malloc(sizeof(Inode));

	fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				currentDir*sizeof(Inode), SEEK_SET);
	fread(parentInode, sizeof(Inode), 1, fp);
	printf("total %d\n", parentInode->i_size);
	//printf("%d %d\n", currentDir,parentInode->i_addr1[0]);
	for (i=0; i<=parentInode->i_size/H; i++)
	{
		for (j=0 ; j<H && (i*H+j)<parentInode->i_size; j++)
		{
		       fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
			     INODE_COUNT*sizeof(Inode)+parentInode->i_addr1[i]*BLOCK_SIZE+j*sizeof(FCB),SEEK_SET);
			fread(fcb, sizeof(FCB), 1, fp);
			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
					fcb->i_num*sizeof(Inode), SEEK_SET);

			fread(fileInode, sizeof(Inode), 1, fp);
				printf("Filename: %-10s	",fcb->filename);
				printf("InodeNum: %-2d	", fcb->i_num);
				printf("user: %-10s", curUser.userName);
				printf("size: %-4d", fileInode->i_size);
				printf("type: %c  ", fileInode->type);
				struct tm *tm_log;
              	tm_log = localtime(&fileInode->t);
            	printf("  modify time: %04d-%02d-%02d %02d:%02d:%02d   ", tm_log->tm_year + 1900, tm_log->tm_mon + 1, tm_log->tm_mday, tm_log->tm_hour, tm_log->tm_min, tm_log->tm_sec);

				if (fcb->i_mode == 1)
				{
					printf("directory\n");
				}
				else
				{
					printf("regular file\n");
				}
		}
	}

	free(fcb);
	free(parentInode);
	
}

void ls()
{
	int i, j;
	FCB *fcb = (FCB *)malloc(sizeof(FCB));
	Inode *fileInode = (Inode *)malloc(sizeof(Inode));
	Inode *parentInode = (Inode *)malloc(sizeof(Inode));

	fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				currentDir*sizeof(Inode), SEEK_SET);
	fread(parentInode, sizeof(Inode), 1, fp);
	for (i=0; i<=parentInode->i_size/H; i++)
	{
		for (j=0 ; j<H && (i*H+j)<parentInode->i_size; j++)
		{
		       fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
			     INODE_COUNT*sizeof(Inode)+parentInode->i_addr1[i]*BLOCK_SIZE+j*sizeof(FCB),SEEK_SET);
			fread(fcb, sizeof(FCB), 1, fp);
			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
					fcb->i_num*sizeof(Inode), SEEK_SET);

			fread(fileInode, sizeof(Inode), 1, fp);
				printf("%-10s\n",fcb->filename);
		}
	}

	free(fcb);
	free(parentInode);
	
}

void mv(char *name1, char *name2)
{
    int fileInodeNum, i, j;
    FCB *fcb = (FCB *)malloc(sizeof(FCB));
	Inode *fileInode = (Inode *)malloc(sizeof(Inode));
	Inode *parentInode = (Inode *)malloc(sizeof(Inode));
        if (((fileInodeNum=findInodeNum(name1,0))==-1)&&((fileInodeNum=findInodeNum(name1,1))==-1))
	   printf("there is no %s file\n", name1);
	else{
		fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				fileInodeNum*sizeof(Inode), SEEK_SET);
		fread(fileInode, sizeof(Inode), 1, fp);

		fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				fileInode->parent*sizeof(Inode), SEEK_SET);
		fread(parentInode, sizeof(Inode), 1, fp);
		strcpy(fileInode->filename, name2);  
    	        time(&(fileInode->t));
		fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				fileInodeNum*sizeof(Inode), SEEK_SET);
		fwrite(fileInode, sizeof(Inode), 1, fp);
		for (i=0; i<=parentInode->i_size/H; i++)
		{
			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
					INODE_COUNT*sizeof(Inode)+parentInode->i_addr1[i]*BLOCK_SIZE, SEEK_SET);
			for (j=0; j<H && (i*H+j)<parentInode->i_size; j++)
			{
				fread(fcb, sizeof(FCB), 1, fp);
       
				if (strcmp(fcb->filename, name1) == 0)
				{
					strcpy(fcb->filename, name2);
					fseek(fp, -sizeof(FCB), SEEK_CUR);
					fwrite(fcb, sizeof(FCB), 1, fp);
					break;
				}
			}
		}
	}
	pFile = fopen("log.txt", "a");
	time_t time_log = time(NULL);
	struct tm* tm_log = localtime(&time_log);
	fprintf(pFile, "%04d-%02d-%02d %02d:%02d:%02d a file named %s changed name %s\n", tm_log->tm_year + 1900, tm_log->tm_mon + 1, tm_log->tm_mday, tm_log->tm_hour, tm_log->tm_min, tm_log->tm_sec, name1, name2);

	fflush(pFile);
	fclose(pFile);
}

  

void updateResource()
{
	rewind(fp);
	fwrite(&superBlock,sizeof(superBlock),1,fp);
	fwrite(BlockBitmap,sizeof(BlockBitmap),1,fp);
	fwrite(InodeBitmap,sizeof(InodeBitmap),1,fp);
	fclose(fp);
}

void login()
{
  char username[20];
  char password[20];
  while(1)
    {
      printf("---please enter username and password---\n");
      printf("username:");
      fgets(username,20,stdin);
      if(username[strlen(username)-1] == '\n')
            username[strlen(username)-1] = '\0';
      system("stty -echo"); //forbid show
      printf("password:");
      fgets(password,20,stdin);
      if(password[strlen(password)-1] == '\n')
            password[strlen(password)-1] = '\0';
      system("stty echo"); //open show
      printf("\n");
      if(strcmp(username, curUser.userName)==0&&strcmp(password,curUser.passWord)==0)
	  break;
    }
}

void chmod(char* name, unsigned char* type)
{
	int fileInodeNum;
	Inode *fileInode = (Inode *)malloc(sizeof(Inode));
	if (((fileInodeNum=findInodeNum(name,0))==-1)&&((fileInodeNum=findInodeNum(name,1))==-1))
	   printf("there is no %s file\n", name);
	else{
		fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				fileInodeNum*sizeof(Inode), SEEK_SET);
		fread(fileInode, sizeof(Inode), 1, fp);
		fileInode->type = type[0];
	        time(&(fileInode->t));
		fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				fileInodeNum*sizeof(Inode), SEEK_SET);
		fwrite(fileInode, sizeof(Inode), 1, fp);
	}
	pFile = fopen("log.txt", "a");
	time_t time_log = time(NULL);
	struct tm* tm_log = localtime(&time_log);
	fprintf(pFile, "%04d-%02d-%02d %02d:%02d:%02d has modified permission of a file named %s\n", tm_log->tm_year + 1900, tm_log->tm_mon + 1, tm_log->tm_mday, tm_log->tm_hour, tm_log->tm_min, tm_log->tm_sec, name);

	fflush(pFile);
	fclose(pFile);
	free(filenode);
}


void showpath()
{
	Inode *curInode = (Inode *)malloc(sizeof(Inode));
	fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				currentDir*sizeof(Inode),SEEK_SET);
	fread(curInode, sizeof(Inode), 1, fp);
	printf("%s@localhost: %s$", curUser.userName, curInode->filename);
	free(curInode);
}


int Aanylse_commmend(char *cmd)
{
    char *syscmd[]={"help","ls", "cd","mkdir","touch","cat","write","rm","mv","chmod", "logout","exit","sysinfo","cls"};
    int i;
    for (i=0; i<5; i++)
        argv[i][0]='\0';
    sscanf(cmd, "%s %s %s %s %s", argv[0], argv[1], argv[2], argv[3], argv[4]);
	for (i=0; i<14; i++)
	{
		if (strcmp(argv[0], syscmd[i])==0)
			return i;
	}
	return 14;
}


void help()
{
	printf("These shell commands are defined internally.  Type `help' to see this list.\n");
	printf("		help:		show command menu\n");
	printf("		sysInfo:	show system infomation\n");
	printf("		ls:         list the digest of the directory's children\n");
	printf("		ls -l:      list the detail of the directory's children\n");
	printf("		cd:	  		change current directory\n");
	printf("		touch:	  	create a regular file\n");
	printf("		mkdir:	 	create a directory\n");
	printf("		cat:		show file content\n");
	printf("		write:	 	write file\n");
	printf("		rm:			delete file or directory\n");
	printf("		mv:	        modify filename\n");
	printf("		chmod:	    modify file permissions\n");
	printf("        logout:     exit user\n");
	printf("		cls:   		clear the screen\n");
	printf("		exit:   	exit the system\n");
}

void sysInfo()
{
    printf("Disk space usage: %.4lf\n", 1 - superBlock.free_block_count/(double)superBlock.block_count);
	printf("block number: %d\n", superBlock.block_count);
	printf("inode number: %d\n", superBlock.inode_count);
	printf("size of block: %d\n", superBlock.block_size);
	printf("free block number: %d\n", superBlock.free_block_count);
	printf("free inode number: %d\n", superBlock.free_inode_count);
}


void commend()
{
	char cmd[20], c;
	int i=0;
	do
	{
        showpath();
        fgets(cmd,20,stdin);
		switch(Aanylse_commmend(cmd))
		{
			case 0:
				help();
				break;
			case 1:
			  if(strcmp(argv[1], "-l")==0)
			    {
				    ls_l();
				    break;
			    }
			    ls(); 
			    break;
			case 2:
				cd(argv[1]);
				break;
			case 3:
				create_file(argv[1],1);
				break;
			case 4:
				create_file(argv[1],0);
				break;
			case 5:
				read(argv[1]);
				break;
			case 6:
				write(argv[1]);
				break;
			case 7:
				delete1(argv[1]);
				break;
	        case 8:
		        mv(argv[1], argv[2]);
				break;
			case 9:
				chmod(argv[1], argv[2]);
				break;
			case 10:
			    updateResource();
				login();
				openFileSystem();
				break;
			case 11:
				exit(0);
				break;
			case 12:
				sysInfo();
				break;
            case 13:
                system("clear");
				break;
			default:
			  printf("No command '%s' found\n", argv[0]);
				break;
		}
	}while(1);
}



int main()
{
    login();
    openFileSystem();
	commend();
	return 0;
}

