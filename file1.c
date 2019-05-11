#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
	unsigned int i_size; //大小
	unsigned int i_blocks; //占多少个块，每个块512字节
	unsigned int parent; //上级目录
	//unsigned int i_links_count; //链接数
	unsigned int i_addr1[12]; //
}Inode;

typedef struct{
	char userName[10];
	char passWord[10];
}User;

typedef struct 
{
	unsigned int i_num;//文件占用的第一个block块号
	char filename[20];
	unsigned int i_mode;
}FCB;


char BlockBitmap[BLOCK_COUNT];
char InodeBitmap[INODE_COUNT];

SuperBlock superBlock;
User curUser = (User){"root", "root"};

unsigned int currentDir; //当前目录
FILE *fp;
Inode *filenode;
char *argv[20];

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
}

void create_file(char *name, int mode)
{
	int curBlock, curInode, i;
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
        
	if (parentInode->i_size == 0)
	{
		//寻找空闲物理块，目前先采用最低效的遍历，后续再考虑其他分配算法
		for	(i=0; i<BLOCK_COUNT; i++)
		{
			if (BlockBitmap[i] == 0)
			{
				curBlock = i;
				BlockBitmap[i] =  1;
				//printf("curblock: %d\n", curBlock);
				break;
			}
		}
		parentInode->i_addr1[0] = curBlock;
		superBlock.free_block_count -= 1;
	}
	fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
			sizeof(Inode)*INODE_COUNT+BLOCK_SIZE*parentInode->i_addr1[0]+
			parentInode->i_size*sizeof(FCB),SEEK_SET);
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
	int fileInodeNum, i;
	Inode *parentInode = (Inode *)malloc(sizeof(Inode));
	FCB *fcb = (FCB *)malloc(sizeof(FCB));

	fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				currentDir*sizeof(Inode), SEEK_SET);
	fread(parentInode, sizeof(Inode), 1, fp);

	fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				INODE_COUNT*sizeof(Inode)+parentInode->i_addr1[0]*BLOCK_SIZE, SEEK_SET);
	for (i=0; i<parentInode->i_size; i++)
	{
		fread(fcb, sizeof(FCB), 1, fp);
		//printf("%s\n", fcb->filename);
		if (mode == 0)
		{
			if (fcb->i_mode == 0 && strcmp(name, fcb->filename)==0)
			{
				fileInodeNum = fcb->i_num;
				break;
			}
		}
		else
		{
			if (fcb->i_mode == 1 && strcmp(name, fcb->filename)==0)
			{
				fileInodeNum = fcb->i_num;
				break;
			}
		}
	}

	if (i==parentInode->i_size) //没找到
		fileInodeNum = -1;

	free(fcb);
	free(parentInode);
	return fileInodeNum;
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
		fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				fileInodeNum*sizeof(Inode), SEEK_SET);
		fread(fileInode, sizeof(Inode), 1, fp);
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
		fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						fileInodeNum*sizeof(Inode), SEEK_SET);
		fwrite(fileInode, sizeof(Inode), 1, fp);
		rewind(fp);
		fwrite(&superBlock, sizeof(SuperBlock), 1, fp);
		fwrite(BlockBitmap, sizeof(BlockBitmap), 1, fp);
	}
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
			printf("read len: %d\n", l);
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
			printf("\nread len: %d\n", l);
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
			printf("\nh:%d\n", h);
			printf("\nread len: %d\n", l);
		}
	}
	printf("\n");
	free(fileInode);
}

void delete1(char *name)
{
        int fileInodeNum, i, j, h;
	int single[BLOCK_SIZE/32];
	int double1[BLOCK_SIZE/32];
	int double2[BLOCK_SIZE/32][BLOCK_SIZE/32];
	Inode *fileInode = (Inode *)malloc(sizeof(Inode));
	Inode *parentInode = (Inode *)malloc(sizeof(Inode));
	FCB fcb[20];
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

			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						INODE_COUNT*sizeof(Inode)+parentInode->i_addr1[0]*BLOCK_SIZE, SEEK_SET);
			for (i=0; i<parentInode->i_size; i++)
			{
				fread(&fcb[i], sizeof(FCB), 1, fp);
			}
			fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
						INODE_COUNT*sizeof(Inode)+parentInode->i_addr1[0]*BLOCK_SIZE, SEEK_SET);
						
			for (i=0; i<parentInode->i_size; i++)
			{
				if((strcmp(fcb[i].filename,name))!=0)
				{
						fwrite(&fcb[i],sizeof(FCB),1,fp);
				}
			}
			rewind(fp);
			fread(&superBlock, sizeof(SuperBlock), 1, fp);
			fread(BlockBitmap, sizeof(BlockBitmap), 1, fp);
			fread(InodeBitmap, sizeof(InodeBitmap), 1, fp);
			if (parentInode->i_size == 1)
			{
				parentInode->i_blocks = 0;
				BlockBitmap[parentInode->i_addr1[0]] = 0;
				superBlock.free_block_count+=1;
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
					for (j=0; j<BLOCK_SIZE/32; j++)
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
			printf("this is not a regular file\n");
		}

	}

	free(fileInode);
	free(parentInode);

}

void list()
{
	int i;
	FCB *fcb = (FCB *)malloc(sizeof(FCB));
	Inode *parentInode = (Inode *)malloc(sizeof(Inode));

	fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				currentDir*sizeof(Inode), SEEK_SET);

	fread(parentInode, sizeof(Inode), 1, fp);
	//intf("i_addr1:%d\n", parentInode->i_addr1[0]);
	fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				INODE_COUNT*sizeof(Inode)+parentInode->i_addr1[0]*BLOCK_SIZE,SEEK_SET);
	for (i=0 ; i<parentInode->i_size; i++)
	{
		fread(fcb, sizeof(FCB), 1, fp);
		printf("Filename: %-10s	",fcb->filename);
		printf("InodeNum: %-2d	", fcb->i_num);
		if (fcb->i_mode == 1)
		{
			printf("directory\n");
		}
		else
		{
			printf("regular file\n");
		}
	}

	free(fcb);
	free(parentInode);
	
}

void cd(char *name)
{
	int fileInodeNum;
	Inode *fileInode = (Inode *)malloc(sizeof(Inode));

	if (strcmp(name, "..") != 0)
	{
		fileInodeNum = findInodeNum(name, 1);
		if (fileInodeNum == -1)
			printf("this is no %s directory\n", name);
		else
			currentDir = fileInodeNum;
	}
	else
	{
		fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				currentDir*sizeof(Inode), SEEK_SET);
		fread(fileInode, sizeof(Inode), 1, fp);
		currentDir = fileInode->parent;
	}

	free(fileInode);
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
/*
void showpath()
{
	char path[100];
	Inode *curInode = (Inode *)malloc(sizeof(Inode));
	fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				currentDir*sizeof(Inode),SEEK_SET);
	fread(curInode, sizeof(Inode), 1, fp);
	strcpy(path, curInode->filename);
	int copy = currentDir;
	currentDir = curInode->parent;
	while(currentDir != 0)
	{
		fseek(fp, sizeof(superBlock)+sizeof(BlockBitmap)+sizeof(InodeBitmap)+
				currentDir*sizeof(Inode),SEEK_SET);
		fread(curInode, sizeof(Inode), 1, fp);
		strcat(curInode->filename, '/');
		strcat(curInode->filename, path);
		strcpy(path, curInode->filename);
		currentDir = curInode->parent;
	}
	currentDir = copy;
	printf("%s@localhost: %s$", curUser.userName, path);
	free(curInode);
}*/

int Aanylse_commmend(char *cmd)
{
	char *syscmd[]={"help","ls","cd","mkdir","touch","cat","write","rm","logout","exit","sysinfo","cls"};
	char *pch;
	pch = strtok(cmd, " ");
	int i=0,j;
	while (pch != NULL)
	{
		argv[i++] = pch;
		pch = strtok(NULL, " ");
	}
	for (i=0; i<12; i++)
	{
		if (strcmp(argv[0], syscmd[i])==0)
			return i;
	}
	return 12;
}


void help()
{
	printf("These shell commands are defined internally.  Type `help' to see this list.\n");
	printf("		help:		show command menu\n");
	printf("		sysInfo:	show system infomation\n");
	printf("		cd:	  		change current directory\n");
	printf("		touch:	  	create a regular file\n");
	printf("		mkdir:	 	create a directory\n");
	printf("		cat:		show file content\n");
	printf("		write:	 	write file\n");
	printf("		rm:			delete file or directory\n");
	printf("		cls:   		clear the screen\n");
	printf("		exit:   	exit\n");
}

void sysInfo()
{
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
		gets(cmd);
		switch(Aanylse_commmend(cmd))
		{
			case 0:
				help();
				break;
			case 1:
				list();
				//intf("\n curdir: %d \n", currentDir);
				break;
			case 2:
				cd(argv[1]);
				//intf("\n curdir: %d \n", currentDir);
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
				//updateResource();
				//login();
				//openFileSystem();
                //command();
				break;
			case 9:
				//updateResource();
				exit(0);
				break;
			case 10:
				sysInfo();
				break;
            case 11:
                system("clear");
				break;
			default:
				break;
		}
	}while(1);
}


int main()
{
        openFileSystem();
	commend();
	return 0;
}




