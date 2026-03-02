#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STATIONS 11
#define MAX_PROJECTS 123
#define INF 999999

//------------------------------------------------模块1：数据读取与数据结构初始化-------------------------------------------------------

//商家项目结构体
typedef struct{
    char name[100];
    char type[20];
    char tag[50];
    int price;
    float open_time;
    float close_time;
    int max_people;
    int min_people;
    char subway_station[100];
    int walk_time;
    float rating;
    float avg_duration;
    int sum_time;
}Project;

//项目开始结束时刻表
typedef struct{
    int start_time;
    int end_time;
}ProjectSchedule;

int subway_matrix[MAX_STATIONS][MAX_STATIONS];
Project projects[MAX_PROJECTS];

//读取地铁邻接矩阵
int load_subway_matrix(const char*filename,int matrix[MAX_STATIONS][MAX_STATIONS]){
    FILE*fp=fopen(filename,"r");
    if(!fp)return 0;

    char line[1024];
    int row=0;

    //读取CSV
    while(fgets(line,sizeof(line),fp)&&row<MAX_STATIONS){
        char*token=strtok(line,",");
        int col=0;
        while(token!=NULL&&col<MAX_STATIONS){
            matrix[row][col]=atoi(token);
            token=strtok(NULL,",");
            col++;
        }
        row++;
    }

    fclose(fp);
    return 1;
}

//读取商家项目数据
int load_project_data(const char*filename,Project projects[]){
    FILE*fp=fopen(filename,"r");
    if(!fp)return 0;

    char line[1024];
    int count=0;

    while(fgets(line,sizeof(line),fp)&&count<MAX_PROJECTS){
        //使用sscanf，读取逗号分割
        sscanf(line,"%[^,],%[^,],%[^,],%d,%f,%f,%d,%d,%[^,],%d,%f,%f",
               projects[count].name,
               projects[count].type,
               projects[count].tag,
               &projects[count].price,
               &projects[count].open_time,
               &projects[count].close_time,
               &projects[count].max_people,
               &projects[count].min_people,
               projects[count].subway_station,
               &projects[count].walk_time,
               &projects[count].rating,
               &projects[count].avg_duration);

        projects[count].open_time=projects[count].open_time*60;
        projects[count].close_time=projects[count].close_time*60;
        projects[count].avg_duration=projects[count].avg_duration*60;
        projects[count].sum_time=0;

        count++;
    }

    fclose(fp);
    return count;
}

//定义并行的站名数组
const char*station_names[MAX_STATIONS]={
    "金沙江路","曹杨路","镇坪路","长寿路","昌平路",
    "武宁路","武定路","隆德路","中山公园","江苏路","静安寺"
};

//辅助函数：通过站名获取对应的矩阵索引(0-10)
int get_station_index(const char*name){
    for(int i=0;i<MAX_STATIONS;i++){
        //使用strcmp进行字符串比较
        if(strcmp(name,station_names[i])==0){
            return i;
        }
    }
    return -1;
}

//floyd计算最短路径
void calculate_floyd(int matrix[MAX_STATIONS][MAX_STATIONS]){
    for(int k=0;k<MAX_STATIONS;k++){
        for(int i=0;i<MAX_STATIONS;i++){
            for(int j=0;j<MAX_STATIONS;j++){
                if(matrix[i][k]+matrix[k][j]<matrix[i][j]){
                    matrix[i][j]=matrix[i][k]+matrix[k][j];
                }
            }
        }
    }
}

//计算从起点（金沙江路）到所有商家的通勤时间
void calculate_all_commutes(Project projects[],int project_count,int matrix[MAX_STATIONS][MAX_STATIONS]){
    //获取起始地铁站序号，0
    int start_idx=get_station_index("金沙江路");

    for(int i=0;i<project_count;i++){
        //获取当前商家所在地铁站序号
        int target_idx=get_station_index(projects[i].subway_station);

        if(start_idx!=-1&&target_idx!=-1){
            //计算地铁时间+步行时间
            projects[i].sum_time=matrix[start_idx][target_idx]+projects[i].walk_time;
        }else{
            //容错处理
            projects[i].sum_time=100000;
        }
    }
}

//----------------------------------模块2：读取用户输入并确定第一个项目----------------------------------
#define MAX_TAGS 50

//获取用户输入
void get_user_inputs(int*group_size,char*activity_type,int*user_start_time,int*user_end_time){
    char time_str[10];
    char time_str_2[10];
    int h,m;

    printf("请输入出行人数: ");
    scanf("%d",group_size);

    printf("请输入活动类型 (饮食/娱乐): ");
    scanf("%s",activity_type);

    printf("请输入您的出发时间 (24小时制,如 09:30 或 17:00): ");
    scanf("%s",time_str);

    //解析字符串
    if(sscanf(time_str,"%d:%d",&h,&m)==2){
        *user_start_time=h*60+m;
    }else{
        printf("时间格式错误，默认设为 09:00\n");
        *user_start_time=9*60;
    }

    printf("请输入您的结束时间 (24小时制,如 09:30 或 17:00): ");
    scanf("%s",time_str_2);

    if(sscanf(time_str_2,"%d:%d",&h,&m)==2){
        *user_end_time=h*60+m;
    }else{
        printf("时间格式错误，默认设为 23:00\n");
        *user_end_time=23*60;
    }
}

//初次筛选，复制初筛商家至新项目集
int filter_projects(Project all[],int all_count,int group_size,const char*type,int user_start_time,Project result[]){
    int count=0;

    for(int i=0;i<all_count;i++){
        //计算预计到达该商家的时间
        int arrival_time=user_start_time+all[i].sum_time;

        //筛选：人数匹配+类型匹配+时间匹配
        if(all[i].min_people<=group_size&&
           all[i].max_people>=group_size&&
           strcmp(all[i].type,type)==0&&
           arrival_time>=(int)all[i].open_time&&
           arrival_time<(int)all[i].close_time){

            //复制符合所有条件的商家到结果集
            result[count]=all[i];
            count++;
        }
    }
    return count;
}

//在新项目集中获取包含的标签
int get_tags(Project filtered[],int filtered_count,char tags[MAX_TAGS][50]){
    int tag_count=0;
    for(int i=0;i<filtered_count;i++){
        int is_duplicate=0;
        //检查该标签是否已存在于标签集中
        for(int j=0;j<tag_count;j++){
            if(strcmp(filtered[i].tag,tags[j])==0){
                is_duplicate=1;
                break;
            }
        }
        //若不重复，则添加
        if(!is_duplicate&&tag_count<MAX_TAGS){
            strcpy(tags[tag_count],filtered[i].tag);
            tag_count++;
        }
    }
    return tag_count;
}

//打印标签集
void print_tags(char tags[][50],int tag_count){
    printf("这个活动类型有以下内容可选择：\n");
    if(tag_count==0){
        printf("（未找到符合条件的内容）\n");
        return;
    }
    for(int i=0;i<tag_count;i++){
        printf("%s\n",tags[i]);
    }
}

//在项目集中搜索用户输入的标签，储存符合标签的项目在final_ps[]，返回存储项目数量
int filter_by_selected_tag(Project current_ps[],int current_count,const char*selected_tag,Project final_ps[]){
    int final_count=0;
    for(int i=0;i<current_count;i++){
        //匹配标签
        if(strcmp(current_ps[i].tag,selected_tag)==0){
            final_ps[final_count]=current_ps[i];
            final_count++;
        }
    }
    return final_count;
}

//可视化函数
int get_display_width(const char*str){
    int width=0;
    while(*str){
        if((unsigned char)*str>127){
            width+=2;
            str+=3;
        }else{
            width+=1;
            str+=1;
        }
    }
    return width;
}

void printf_aligned(const char*str,int target_width){
    int current_width=get_display_width(str);
    printf("%s",str);
    for(int i=0;i<target_width-current_width;i++){
        printf(" ");
    }
}

//打印符合标签的项目
void print_final_projects(Project final_ps[],int final_count){
    printf("\n为您找到以下符合 [%s] 种类的商家：\n",final_ps[0].tag);
    printf("--------------------------------------------------------------------------------\n");

    //表头对齐
    printf("%-6s","序号");
    printf_aligned("店名",25);
    printf_aligned(" 最近地铁站",15);
    printf("%-10s\n","人均价格");

    for(int i=0;i<final_count;i++){
        //序号
        printf("[%2d]  ",i+1);

        //店名(目标宽度25)
        printf_aligned(final_ps[i].name,25);

        //最近地铁站(目标宽度15)
        printf_aligned(final_ps[i].subway_station,15);

        //价格(数字通常是对齐的)
        printf("%-10d\n",final_ps[i].price);
    }
    printf("--------------------------------------------------------------------------------\n");
}

//计算第一次活动的开始与结束时间
void calculate_first_schedule(int user_start_time,Project first_project,ProjectSchedule*first_schedule){
    first_schedule->start_time=user_start_time+first_project.sum_time;
    first_schedule->end_time=first_schedule->start_time+(int)first_project.avg_duration;
}

//--------------------------------------------模块3：连续规划-----------------------------------------------------
#define LUNCH_START 660
#define LUNCH_END 840
#define DINNER_START 1020
#define DINNER_END 1200

//检查标签是否已在今日行程中出现过
int is_tag_duplicate(const char*tag,Project today_ps[],int today_count){
    for(int i=0;i<today_count;i++){
        if(strcmp(tag,today_ps[i].tag)==0)return 1;
    }
    return 0;
}

//计算两个项目之间的通勤时间
int get_travel_time(Project p1,Project p2,int matrix[MAX_STATIONS][MAX_STATIONS]){
    int s1=get_station_index(p1.subway_station);
    int s2=get_station_index(p2.subway_station);
    if(s1==-1||s2==-1)return INF;
    return p1.walk_time+matrix[s1][s2]+p2.walk_time;
}

//返回值：0不吃饭，1吃饭
int check_eating_time(int current_time){
    if((current_time>=LUNCH_START&&current_time<=LUNCH_END)||
       (current_time>=DINNER_START&&current_time<=DINNER_END)){
        return 1;
    }
    return 0;
}

//餐饮推荐函数
Project recommend_meal(Project all[],int all_count,Project prev_p,Project today_ps[],int today_cnt,int matrix[MAX_STATIONS][MAX_STATIONS],int prev_end_time,int user_end_time){
    Project eating_projects[MAX_PROJECTS];
    int e_count=0;

    //局部剪枝变量定义
    Project same_station_project[MAX_PROJECTS];
    int same_station_project_count=0;
    int same_station_max=-1;
    int same_station_min=INF;

    //获取上一个项目的地铁站序号
    int prev_idx=get_station_index(prev_p.subway_station);
    
    //提取同站商家，查找同站步行时间极值
    for(int i=0;i<all_count;i++){
        if(strcmp(all[i].subway_station,prev_p.subway_station)==0){
            same_station_project[same_station_project_count]=all[i];
            if(all[i].walk_time>same_station_max)same_station_max=all[i].walk_time;
            if(all[i].walk_time<same_station_min)same_station_min=all[i].walk_time;
            same_station_project_count++;
        }
    }

    //查找其他站点的通勤时间极值
    int other_station_min=INF;
    int other_station_max=-1;
    for(int i=0;i<MAX_STATIONS;i++){
        if(i==prev_idx)continue;
        if(matrix[prev_idx][i]<other_station_min)other_station_min=matrix[prev_idx][i];
        if(matrix[prev_idx][i]>other_station_max)other_station_max=matrix[prev_idx][i];
    }


    //局部剪枝
    Project*search_pool;
    int search_count;
    if(same_station_max<=other_station_min&&same_station_project_count>0){
        //满足剪枝条件：同站最长步行时间<=跨站最短地铁乘坐时间
        search_pool=same_station_project;
        search_count=same_station_project_count;
        //printf("剪枝成功\n");
    }else{
        //不满足剪枝条件，搜索全部
        search_pool=all;
        search_count=all_count;
        //printf("剪枝失败\n");
    }
    //局部剪枝结束

    for(int i=0;i<search_count;i++){
        //计算预估结束时间:上个项目结束时间+通勤时间+该项目游玩时长
        int travel=get_travel_time(prev_p,search_pool[i],matrix);
        int potential_end=prev_end_time+travel+(int)search_pool[i].avg_duration;

        //筛选:类型匹配+标签不重复+结束时间不严重超时
        if(strcmp(search_pool[i].type,"饮食")==0&&
           !is_tag_duplicate(search_pool[i].tag,today_ps,today_cnt)&&
           potential_end<=(user_end_time+60)){

            eating_projects[e_count]=search_pool[i];
            eating_projects[e_count].sum_time=travel;
            e_count++;
        }
    }

    //冒泡排序
    if(e_count==0)return(Project){0};
    for(int i=0;i<e_count-1;i++){
        for(int j=0;j<e_count-i-1;j++){
            if(eating_projects[j].sum_time>eating_projects[j+1].sum_time||
               (eating_projects[j].sum_time==eating_projects[j+1].sum_time&&
                eating_projects[j].rating<eating_projects[j+1].rating)){
                Project temp=eating_projects[j];
                eating_projects[j]=eating_projects[j+1];
                eating_projects[j+1]=temp;
            }
        }
    }
    return eating_projects[0];
}


//娱乐推荐函数
Project recommend_playing(Project all[],int all_count,Project prev_p,Project today_ps[],int today_cnt,int matrix[MAX_STATIONS][MAX_STATIONS],int prev_end_time,int user_end_time){
    Project playing_projects[MAX_PROJECTS];
    int p_count=0;

    Project same_station_project[MAX_PROJECTS];
    int same_station_project_count=0;
    int same_station_max=-1;
    int same_station_min=INF;

    
    int prev_idx=get_station_index(prev_p.subway_station);

   
    for(int i=0;i<all_count;i++){
        if(strcmp(all[i].subway_station,prev_p.subway_station)==0){
            same_station_project[same_station_project_count]=all[i];
            if(all[i].walk_time>same_station_max)same_station_max=all[i].walk_time;
            if(all[i].walk_time<same_station_min)same_station_min=all[i].walk_time;
            same_station_project_count++;
        }
    }

    int other_station_min=INF;
    for(int i=0;i<MAX_STATIONS;i++){
        if(i==prev_idx)continue;
        if(matrix[prev_idx][i]<other_station_min)other_station_min=matrix[prev_idx][i];
    }

    Project*search_pool;
    int search_count;
    if(same_station_max<=other_station_min&&same_station_project_count>0){
        search_pool=same_station_project;
        search_count=same_station_project_count;
        //printf("剪枝成功\n");
    }else{
        search_pool=all;
        search_count=all_count;
        //printf("剪枝失败\n");
    }
    for(int i=0;i<search_count;i++){
        int travel=get_travel_time(prev_p,search_pool[i],matrix);
        int potential_end=prev_end_time+travel+(int)search_pool[i].avg_duration;

        //筛选
        if(strcmp(search_pool[i].type,"娱乐")==0&&
           !is_tag_duplicate(search_pool[i].tag,today_ps,today_cnt)&&
           potential_end<=(user_end_time+60)){

            playing_projects[p_count]=search_pool[i];
            playing_projects[p_count].sum_time=travel;
            p_count++;
        }
    }

    //冒泡排序
    if(p_count==0)return(Project){0};
    for(int i=0;i<p_count-1;i++){
        for(int j=0;j<p_count-i-1;j++){
            if(playing_projects[j].sum_time>playing_projects[j+1].sum_time||
               (playing_projects[j].sum_time==playing_projects[j+1].sum_time&&
                playing_projects[j].rating<playing_projects[j+1].rating)){
                Project temp=playing_projects[j];
                playing_projects[j]=playing_projects[j+1];
                playing_projects[j+1]=temp;
            }
        }
    }
    return playing_projects[0];
}

//存储下一个项目的开始时间与结束时间
void calculate_next_schedule(Project prev_p,Project next_p,ProjectSchedule*next_s,int matrix[MAX_STATIONS][MAX_STATIONS],int prev_end_time){
    int travel=get_travel_time(prev_p,next_p,matrix);
    next_s->start_time=prev_end_time+travel;
    next_s->end_time=next_s->start_time+(int)next_p.avg_duration;
}

//出行方案可视化
//转换为24小时制显示
void display_time_hhmm(int total_minutes){
    int hours=(total_minutes/60)%24;
    int minutes=total_minutes%60;
    printf("%02d:%02d",hours,minutes);
}

//计算今日行程的总预计消费
int calculate_total_expense(Project today_ps[],int today_cnt){
    int total=0;
    for(int i=0;i<today_cnt;i++){
        total+=today_ps[i].price;
    }
    return total;
}

//------------------------------------------主程序框架---------------------------------------------------------------------
int main(){

    //加载地铁数据
    if(load_subway_matrix("subwaystation_distance_initial.csv",subway_matrix)){
        printf("地铁邻接矩阵加载成功。\n");
        //调用floyd更新
        calculate_floyd(subway_matrix);
        printf("地铁全网最短路径计算完成。\n");
    }else{
        printf("地铁文件读取失败！\n");
        return -1;
    }

    //加载商家项目数据
    int count=load_project_data("data_list.csv",projects);
    if(count>0){
        printf("商家数据加载成功，共 %d 条。\n",count);
    }else{
        printf("商家数据读取失败，请检查文件名！\n");
    }

    calculate_all_commutes(projects,count,subway_matrix);

    //全局变量设定
    Project today_projects[10];
    ProjectSchedule today_schedules[10];

    //读取第一次用户输入
    int today_count=0;
    int group_size;
    char activity_type[20];
    int user_start_time=0;
    int user_end_time=0;

    Project current_projects[MAX_PROJECTS];
    char current_tags[MAX_TAGS][50];

    get_user_inputs(&group_size,activity_type,&user_start_time,&user_end_time);

    //筛选项目标签和数量
    int filtered_count=filter_projects(projects,MAX_PROJECTS,group_size,activity_type,user_start_time,current_projects);

    int tag_count=get_tags(current_projects,filtered_count,current_tags);
    print_tags(current_tags,tag_count);

    //定义存储符合标签的项目的变量
    char selected_tag[50];
    Project final_current_projects[MAX_PROJECTS];
    int final_count=0;

    //读取用户输入的标签
    printf("请输入您想去的种类名称: ");
    scanf("%s",selected_tag);
    final_count=filter_by_selected_tag(current_projects,filtered_count,selected_tag,final_current_projects);

    if(final_count==0){
        printf("抱歉，未找到匹配类别的活动项目。\n");
    }else{
        //打印项目列表供用户选择
        print_final_projects(final_current_projects,final_count);

        //读取用户输入的活动项目
        int choice_idx;
        printf("\n请输入您想去的商家序号 (1-%d): ",final_count);
        scanf("%d",&choice_idx);

        if(choice_idx>=1&&choice_idx<=final_count){
            //存储到今日行程today_projects中
            today_projects[today_count]=final_current_projects[choice_idx-1];

            today_count++;
        }else{
            printf("输入序号无效。\n");
        }
    }

    //假设用户已经选定了第一个项目并存储在today_projects[0]
    if(today_count>0){
        //调用函数计算第一个项目的开始时间和结束时间
        calculate_first_schedule(user_start_time,today_projects[0],&today_schedules[0]);

        //可视化
        printf("------------------------------------------\n");
        printf("第一个地点: %s\n",today_projects[0].name);
        printf("需乘往地铁站%s,预计活动持续时间: ",today_projects[0].subway_station);
        display_time_hhmm(today_schedules[0].start_time);
        printf(" - ");
        display_time_hhmm(today_schedules[0].end_time);
        printf("\n");
        printf("------------------------------------------\n");
    }

    //循环规划下一个项目
    while(today_count<10){

        int current_finish_time=today_schedules[today_count-1].end_time;

        //判断是否超过今日结束时间
        if(current_finish_time>user_end_time){
            printf("行程已达到设定的结束时间，规划完成。\n");
            break;
        }

        //规划活动
        Project next_one;

        if(check_eating_time(current_finish_time)&&strcmp(today_projects[today_count-1].type,"饮食")!=0){
            next_one=recommend_meal(projects,count,today_projects[today_count-1],
                                    today_projects,today_count,subway_matrix,
                                    current_finish_time,user_end_time);
        }else{
            next_one=recommend_playing(projects,count,today_projects[today_count-1],
                                       today_projects,today_count,subway_matrix,
                                       current_finish_time,user_end_time);
        }

        if(next_one.name[0]=='\0'){
            printf("没有符合条件的后续项目了~规划结束！\n");
            break;
        }


        //存入today_project
        today_projects[today_count]=next_one;

        //计算并存入today_schedule
        calculate_next_schedule(today_projects[today_count-1],today_projects[today_count],
                                &today_schedules[today_count],subway_matrix,
                                today_schedules[today_count-1].end_time);

        //可视化结果
        printf("规划下一个地点: %s,种类: %s\n",today_projects[today_count].name,today_projects[today_count].tag);
        printf("需乘往地铁站%s,预计活动持续时间: ",today_projects[today_count].subway_station);
        display_time_hhmm(today_schedules[today_count].start_time);
        printf(" - ");
        display_time_hhmm(today_schedules[today_count].end_time);
        printf("\n");
        printf("------------------------------------------\n");

        //计数
        today_count++;
    }
    int total_expense=calculate_total_expense(today_projects,today_count);

    printf("------------------------------------------\n");
    printf("今日行程规划总结：\n");
    printf(">> 计划总项目数：%d 个\n",today_count);
    printf(">> 预计总消费金额：%d 元\n",total_expense);

    return 0;
}