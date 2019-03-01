#include<stdio.h>
#include<ctype.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
struct jcb
{
        char name[10];//作业名     
	    char status;//状态
	        int arrtime;//到达时间
		    int reqtime;//要求服务时间
		        int startime;//调度时间
			    int finitime;//完成时间
			        float waittime;//等待时间
				    float TAtime;//周转时间
				        float TAWtime;//带权周转时间
					    float rp;//响应比
					        int ip;
}job[24];
void sort(struct jcb temp[24],int count)
{
        int i,j;
	    struct jcb t;
	        for(i=0;i<count-1;i++) 
		        {                               
			            for(j=i+1;j<count;j++)
					        { 
						                if(temp[j].arrtime< temp[i].arrtime)
								                {
										                    t=temp[j];
												                    temp[j]=temp[i];
														                    temp[i]=t;
																                }

								        }
				        }
		    
}
void FCFS(struct jcb temp[24],int count)
{
        int i=0;
	    float averageruntime=0;
	        float averageruntimerun=0;
		    sort(temp,count);
		        temp[i].startime =temp[i].arrtime;
			    temp[i].finitime =temp[i].reqtime +temp[i].startime ;
			        temp[i].TAtime =temp[i].finitime -temp[i].arrtime ;
				    temp[i].TAWtime =temp[i].TAtime/temp[i].reqtime;
				        averageruntime=temp[i].TAtime;
					    averageruntimerun=temp[i].TAWtime;
					        for(i=1;i<count;i++)
						        {
							            temp[i].startime =temp[i-1].finitime ;
								            temp[i].finitime =temp[i].reqtime +temp[i].startime ;
									            temp[i].TAtime =temp[i].finitime -temp[i].arrtime ;
										            temp[i].TAWtime =temp[i].TAtime/temp[i].reqtime;
											            averageruntime+=temp[i].TAtime;
												            averageruntimerun+=temp[i].TAWtime;

													        }
						    printf("\t作业名 到达时间 要求服务时间 调度时间 完成时间 周转时间 带权周转时间\n");
						        for(i=0;i<count;i++)
							        {
								            printf("N%d\t%s\t%d\t%d\t%d\t%d\t%f\t%f\n",i+1,job[i].name ,job[i].arrtime ,job[i].reqtime,job[i].startime ,job[i].finitime ,job[i].TAtime ,job[i].TAWtime  );
									        }
							    printf("平均周转时间是：%f\n",averageruntime/count);
							        printf("平均周转时间是：%f\n",averageruntimerun/count);


}

void SJF(struct jcb temp[24],int count)
{
         
        int i=0,j;
	    struct jcb t;
	        float averageruntime=0;
		    float averageruntimerun=0;
		        sort(temp,count);
			    temp[i].startime =temp[i].arrtime;
			        temp[i].finitime =temp[i].reqtime +temp[i].startime ;
				    temp[i].TAtime =temp[i].finitime -temp[i].arrtime ;
				        temp[i].TAWtime =temp[i].TAtime/temp[i].reqtime;
					    averageruntime=temp[i].TAtime;
					        averageruntimerun=temp[i].TAWtime;
						    for(i=1;i<count-1;i++) 
							    {                               
								        for(j=i+1;j<count;j++)
									            { 
											            if(temp[j].reqtime< temp[i].reqtime)
													            {
															                t=temp[j];
																	                temp[j]=temp[i];
																			                temp[i]=t;
																					            }

												            }
									    }
						        for(i=1;i<count;i++)
							        {
								            temp[i].startime =temp[i-1].finitime ;
									            temp[i].finitime =temp[i].reqtime +temp[i].startime ;
										            temp[i].TAtime =temp[i].finitime -temp[i].arrtime ;
											            temp[i].TAWtime =temp[i].TAtime/temp[i].reqtime;
												            averageruntime+=temp[i].TAtime;
													            averageruntimerun+=temp[i].TAWtime;


														        }
							    printf("\t作业名 到达时间 要求服务时间 调度时间 完成时间 周转时间 带权周转时间\n");
							        for(i=0;i<count;i++)
								        {
									            printf("N%d\t%s\t%d\t%d\t%d\t%d\t%f\t%f\n",i+1,job[i].name ,job[i].arrtime ,job[i].reqtime,job[i].startime ,job[i].finitime ,job[i].TAtime ,job[i].TAWtime  );
										        }
								    printf("平均周转时间是：%f\n",averageruntime/count);
								        printf("平均周转时间是：%f\n",averageruntimerun/count);

									    
}

void HRRN(struct jcb temp[24],int count)
{
        int i=0,j;
	    struct jcb t;
	        float averageruntime=0;
		    float averageruntimerun=0;
		        sort(temp,count);
			    temp[i].startime =temp[i].arrtime;
			        temp[i].finitime =temp[i].reqtime +temp[i].startime ;
				    temp[i].TAtime =temp[i].finitime -temp[i].arrtime ;
				        temp[i].TAWtime =temp[i].TAtime/temp[i].reqtime;
					    temp[i].waittime=temp[i].startime-temp[i].arrtime;

					        temp[i].rp=temp[i].TAtime/temp[i].reqtime;
						    averageruntime=temp[i].TAtime;
						        averageruntimerun=temp[i].TAWtime;
							    for(i=1;i<count;i++)
								    {
									        temp[i].startime =temp[i-1].finitime ;
										        temp[i].finitime =temp[i].reqtime +temp[i].startime ;
											        temp[i].waittime=temp[i].startime-temp[i].arrtime;
												        temp[i].TAtime =temp[i].finitime -temp[i].arrtime ;
													        temp[i].TAWtime =temp[i].TAtime/temp[i].reqtime;
														        temp[i].rp=temp[i].TAtime/temp[i].reqtime;
															        

															    }
							        for(i=1;i<count-1;i++) 
								        {                               
									            for(j=i+1;j<count;j++)
											        { 
												                if(temp[j].rp< temp[i].rp)
														                {
																                    t=temp[j];
																		                    temp[j]=temp[i];
																				                    temp[i]=t;
																						                }

														        }
										        }
								    for(i=1;i<count;i++)
									    {
										        temp[i].startime =temp[i-1].finitime ;
											        temp[i].finitime =temp[i].reqtime +temp[i].startime ;
												        temp[i].TAtime =temp[i].finitime -temp[i].arrtime ;
													        temp[i].TAWtime =temp[i].TAtime/temp[i].reqtime;
														        averageruntime+=temp[i].TAtime;
															        averageruntimerun+=temp[i].TAWtime;


																    }
								        printf("\t作业名 到达时间 要求服务时间 调度时间 完成时间 周转时间 带权周转时间\n");
									    for(i=0;i<count;i++)
										    {
											        printf("N%d\t%s\t%d\t%d\t%d\t%d\t%f\t%f\n",i+1,job[i].name ,job[i].arrtime ,job[i].reqtime,job[i].startime ,job[i].finitime ,job[i].TAtime ,job[i].TAWtime  );
												    }
									        printf("平均周转时间是：%f\n",averageruntime/count);
										    printf("平均周转时间是：%f\n",averageruntimerun/count);

										        
}
int ReadFile()
{
       
        int i=0;
	    FILE *fp;     
	        fp=fopen("3.txt","r");  
		    if(fp==NULL)
			    {
				        printf("File open error !\n");
					        exit(0);
						    }
		        printf("\n id    作业到达时间     作业运行所需要时间\n");
			    while(!feof(fp))
				    {
					        fscanf(fp,"%s%d%d",&job[i].name,&job[i].arrtime,&job[i].reqtime); 
						        printf("\n%3s%12d%15d",job[i].name,job[i].arrtime,job[i].reqtime); 
							        i++;
								    };

			        if(fclose(fp))  
				        {
					            printf("Can not close the file !\n");
						            exit(0);
							        }
				   
				    return i;

}


int Pseudo_random_number()
{
        int i,n;
	    srand((unsigned)time(0));
	        n=rand()%23+5;
		    for(i=0; i<n; i++)
			    {
				        job[i].ip=i+1;
					        job[i].arrtime=rand()%29+1;
						        job[i].reqtime=rand()%7+1;
							    }
		        printf("\n id    作业到达时间     作业运行所需要时间\n");
			    for(i=0; i<n; i++)
				    {
					        printf("\n%3d%12d%15d",job[i].ip,job[i].arrtime,job[i].reqtime);
						    }
			        return n;

}


int main()
{ 
        int i,count,select;
	    printf("**************************************************\n");
	        printf("1、调用文本写入数据\n");
		    printf("2、调用伪随机数的产生数据\n");
		        printf("3、调用自己输入模拟数据\n");
			    printf("**************************************************\n\n");
			        printf("请选择菜单项：");
				    scanf("%d",&select);
				        while(select<1||select>3)
					        {
						            printf("输入有误，请重新输入:");
							            scanf("%d",&select);
								        }
					    if(select==1)
						    {
							        count=ReadFile();
								    }
					        else if(select==2)
						        {
							           count=Pseudo_random_number();
								       }
						    else if(select==3)
							    {
								        printf("请输入作业个数：");
									        scanf("%d",&count);
										        while(count<2&&count>24)
											            {
													            printf("输入有误，请重新输入:");
														                scanf("%d",&count);
																        
																        }

											        printf("\n\n");
												        for(i=0;i<count;i++)
													            {
															        printf("第%d个作业：\n",i+1);
																        printf("输入作业名：");
																	        scanf("%s",job[i].name);
																		        printf("到达时间：" );
																			        scanf("%d",&job[i].arrtime);
																				        printf("要求服务时间：");
																					        scanf("%d",&job[i].reqtime );
																						        printf("\n");
																							        }
													        printf("经按到达时间排序后，未达到队列是\n");
														        printf("\t作业名\t到达时间\t要求服务时间\n");
															        for(i=0;i<count;i++)
																            {
																		        printf("N%d\t%s\t%d\t%d\n",i+1,job[i].name ,job[i].arrtime ,job[i].reqtime);
																			        }

																    }
						        while(1){
							        printf("\n---------------请选择作业调度算法----------------\n");
								    printf("1:采用先来先服务 (FCFS) 调度算法\n2:采用短作业优先 (SJF) 调度算法\n3:采用响应比高者优先 (HRRN) 调度算法\n0:退出算法调度");
								        printf("\n**************************************************\n");
									    printf("请选择菜单项：");
									        scanf("%d",&select);
										    while(select<0||select>3)
											    {
												        printf("输入有误，请重新输入:");
													        scanf("%d",&select);
														        
														    }

										        if(select==1)
											        {
												               printf("------先来先服务 (FCFS) 调度算法------\n");
													               FCFS(job,count);

														           }
											    else if(select==2)
												    {
													            printf("------短作业优先 (SJF)调度算法------\n");
														                SJF(job,count);
																       
																    }
											        else if(select==3)
												        {
													            printf("------响应比高者优先 (HRRN)调度算法------\n");
														            HRRN(job,count);
															           
															        }
												    else if(select==0)
													        exit(0);
												        }

							    return 0;
}
