#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <math.h>

//本程序基于文件级指令调用Lowtran7并获取有效数据

double data_lowtran_transmit[6600][14];//定义0.3~30微米波段大气透过率数据存储器，第一列为波数（cm-1）；第二列为波长（微米）；第三列为大气总透过率
double pi = 3.14159265358979;//定义圆周率

void judgement_invoke_lowtran(int run_x_coordinate, int run_y_coordinate)//本函数用于测试lowtran是否能被正常调用//本函数测试通过
{
    remove("Tape5");
    remove("TAPE6");
    remove("TAPE7");
    remove("TAPE8");

    FILE* fp_Tape5;//创建Tape5文件，尝试调用lowtran
    fp_Tape5 = fopen("Tape5.dat", "w");
    if (fp_Tape5 == NULL)
    {
        printf("无法打开文件\n");
        exit(0);
    }

    fprintf(fp_Tape5, "1    1    0    0    0    0    0    0    0    0    0    1    0   0.000   0.80\n");//输入第一行数据
    fprintf(fp_Tape5, "0    0    1    0    0    0     0.000     0.000     0.000     0.000     0.000\n");//输入第二行数据
    fprintf(fp_Tape5, "0.500     0.000     0.000     0.500     0.000     0.000    0\n");//输入第三行数据
    fprintf(fp_Tape5, "2000.000  3333.000     5.000\n");//输入第四行数据
    fprintf(fp_Tape5, "0");//输入第五行数据

    fclose(fp_Tape5);

    rename("Tape5.dat", "Tape5");//将生成的Tape5.dat文件转换为lowtran可读文件

    int judgement_lowtran = 0;
    judgement_lowtran = system("start Lowtran.exe");//运行lowtran

    if (judgement_lowtran != 0)
    {
        printf("错误：无法打开Lowtran.exe\n");
        exit(0);
    }

    Sleep(1000);
    SetCursorPos(run_x_coordinate, run_y_coordinate);//模拟Lowtran运行过程
    mouse_event(MOUSEEVENTF_LEFTDOWN, run_x_coordinate, run_y_coordinate, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, run_x_coordinate, run_y_coordinate, 0, 0);
    Sleep(1000);

    system("taskkill /IM lowtran.exe /F");//关闭lowtran

    FILE* fp_TAPE6_exist;
    FILE* fp_TAPE7_exist;
    FILE* fp_TAPE8_exist;

    fp_TAPE6_exist = fopen("TAPE6", "r");
    if (fp_TAPE6_exist == NULL)
    {
        printf("错误：Lowtran7调用失败\n");
        exit(0);
    }
    fclose(fp_TAPE6_exist);

    fp_TAPE7_exist = fopen("TAPE7", "r");
    if (fp_TAPE7_exist == NULL)
    {
        printf("错误：Lowtran7调用失败\n");
        exit(0);
    }
    fclose(fp_TAPE7_exist);

    fp_TAPE8_exist = fopen("TAPE8", "r");
    if (fp_TAPE8_exist == NULL)
    {
        printf("错误：Lowtran7调用失败\n");
        exit(0);
    }
    fclose(fp_TAPE8_exist);

    printf("提示：Lowtran7可正常调用！\n");

    remove("Tape5");
    remove("TAPE6");
    remove("TAPE7");
    remove("TAPE8");
}

void run_lowtran_transmit(double detect_distance_2, double detect_zenith_angle_2, double target_height)//本函数用于模拟lowtran运行，并获取大气透过率TAPE6文件
{
    int run_x_coordinate = 0;//定义lowtran运行按钮所在坐标
    int run_y_coordinate = 0;

    FILE* fp_RunCoordinate;
    fp_RunCoordinate = fopen("run_lowtran", "r");

    if (fp_RunCoordinate == NULL)
    {
        printf("错误：无法打开文件run_lowtran\n");
        exit(0);
    }

    fscanf(fp_RunCoordinate, "%d %d", &run_x_coordinate, &run_y_coordinate);

    fclose(fp_RunCoordinate);

    judgement_invoke_lowtran(run_x_coordinate, run_y_coordinate);//在正式调用lowtran之前先判断lowtran能否正常运行


    //输入第一行数据
    int model_atmosphere = 0;//定义大气模式

    printf("*****请输入大气模式：\n");
    printf("1 热带大气\n");
    printf("2 中纬度夏季\n");
    printf("3 中纬度冬季\n");
    printf("4 副极带夏季\n");
    printf("5 副极带冬季\n");
    printf("6 美国标准大气（1976年）\n");
    printf("当前选择：");
    scanf("%d", &model_atmosphere);

    if (((model_atmosphere) < 1) || ((model_atmosphere) > 6))
    {
        printf("错误：大气模式选择不规范\n");
        exit(0);
    }

    int type_path_atmosphere = 0;//定义大气几何路径类型，其中1代表水平等压路径；2代表两点确定的斜程；3代表一点确定的斜程

    if ((detect_zenith_angle_2 < 90.000001) && (detect_zenith_angle_2 > 89.999999))
    {
        type_path_atmosphere = 1;
    }
    else
    {
        type_path_atmosphere = 2;
    }

    //第三到第十个数字默认为0
    //第十一、十二个数字默认为1
    //第十三个数字默认为0
    //第十四个数字为边界温度，默认为0.000
    //第十五个数字为地面反照率，默认为0.8

    //输入第二行数据
    int type_aerosol_boundary = 0;//定义边界层气溶胶类型

    printf("*****请输入边界层气溶胶类型：\n");
    printf("0 无气溶胶\n");
    printf("1 乡村消光系数（VIS=23km）\n");
    printf("2 乡村消光系数（VIS=5km）\n");
    printf("3 海洋消光系数\n");//因舍弃了3，故实际为4
    printf("4 城市消光系数\n");//实际为5
    printf("5 对流层消光系数\n");//实际为6
    printf("6 对流雾消光系数\n");//因舍弃了自定义7，故实际为8
    printf("7 辐射雾消光系数\n");//实际为9
    printf("8 沙漠消光系数\n");//实际为10
    printf("当前选择：");
    scanf("%d", &type_aerosol_boundary);

    if ((type_aerosol_boundary < 0) || (type_aerosol_boundary > 8))
    {
        printf("错误：边界层气溶胶类型选择不规范\n");
        exit(0);
    }

    if ((type_aerosol_boundary > 2) && (type_aerosol_boundary) < 6)
    {
        type_aerosol_boundary = type_aerosol_boundary + 1;
    }
    else
    {
        if (type_aerosol_boundary > 5)
        {
            type_aerosol_boundary = type_aerosol_boundary + 2;
        }
    }

    int type_aerosol_trait = 0;//定义气溶胶季节特点

    printf("*****请输入气溶胶季节特点：\n");
    printf("0 由大气模式决定\n");
    printf("1 春-夏季\n");
    printf("2 秋-冬季\n");
    printf("当前选择：");
    scanf("%d", &type_aerosol_trait);

    if ((type_aerosol_trait < 0) || (type_aerosol_trait > 3))
    {
        printf("错误：气溶胶季节模式选择不规范\n");
        exit(0);
    }

    int type_stratosphere = 0;//定义平流层类型//1：背景平流层廓线和消光系数//2：中等火山廓线和旧火山消光系数//3：强烈火山廓线和新火山消光系数//4：强烈火山廓线和旧火山消光系数//5：中等火山廓线和新火山消光系数//6：中等火山廓线和背景火山消光系数//7：强烈火山廓线和背景火山消光系数//8：极强火山廓线和新火山消光系数

    printf("*****请输入平流层廓线和消光系数（30~100km）：\n");
    printf("1 背景平流层廓线和消光系数\n");
    printf("2 中等火山廓线和旧火山消光系数\n");
    printf("3 强烈火山廓线和新火山消光系数\n");
    printf("4 强烈火山廓线和旧火山消光系数\n");
    printf("5 中等火山廓线和新火山消光系数\n");
    printf("6 中等火山廓线和背景平流层消光系数\n");
    printf("7 强烈火山廓线和背景平流层消光系数\n");
    printf("8 极强火山廓线和新火山消光系数\n");
    printf("当前选择：");
    scanf("%d", &type_stratosphere);

    if ((type_stratosphere < 1) || (type_stratosphere > 8))
    {
        printf("错误：平流层廓线和消光系数选择不规范\n");
        exit(0);
    }

    //第四个数字默认为0

    int model_cloud = 0;//定义云模式//目前仅支持模式0~5，其余模式需要手动输入数据

    printf("*****请输入云模式：\n");
    printf("0 无云或雨\n");
    printf("1 积状云\n");
    printf("2 高层云\n");
    printf("3 层云\n");
    printf("4 层积云\n");
    printf("5 雨层云\n");
    printf("当前选择：");
    scanf("%d", &model_cloud);

    if ((model_cloud < 0) || (model_cloud > 5))
    {
        printf("错误：云模式选择不规范\n");
        exit(0);
    }

    //第六个数字默认为0
    //第七、八、九、十、十一个数字默认为0.000

    //输入第三行数据

    double initial_height = 0;//定义初始高度，单位km
    double terminal_height = 0;//定义终点高度，单位km
    double initial_zenith = 0;//定义初始天顶角，单位°
    double path_length = 0;//定义路径长度，单位km

    double Earth_centre_angle = 0;//定义地球中心角，单位°
    double Earth_radius = 6371;//定义地球半径，单位km

    if (type_path_atmosphere == 1)
    {
        initial_height = target_height * 0.001;//将米制转换为千米制
        path_length = detect_distance_2 * 0.001;//将米制转换为千米制
    }
    else
    {
        initial_height = (target_height + detect_distance_2 * cos(detect_zenith_angle_2 * pi / 180)) * 0.001;//此时初始高度为探测器真实高度，即目标高度加探测高度，探测高度由探测距离和探测天顶角引起
        initial_zenith = detect_zenith_angle_2;
        path_length = detect_distance_2 * 0.001;//将米制转换为千米制
    }

    //第七个数字默认为0

    //输入第四行数据
    double start_wave_num = 330;//定义起始波数
    double end_wave_num = 33330;//定义结束波数
    double gap_wave_num = 5;//定义间隔波数

    //输入第五行数据
    //第五行只有一个数，默认为0

    FILE* fp_Tape5;
    fp_Tape5 = fopen("Tape5.dat", "w");
    if (fp_Tape5 == NULL)
    {
        printf("无法打开文件\n");
        exit(0);
    }

    fprintf(fp_Tape5, "%d %d 0 0 0 0 0 0 0 0 1 1 0 0.000 0.8\n", model_atmosphere, type_path_atmosphere);//输入第一行数据
    fprintf(fp_Tape5, "%d %d %d 0 %d 0 0.000 0.000 0.000 0.000 0.000\n", type_aerosol_boundary, type_aerosol_trait, type_stratosphere, model_cloud);//输入第二行数据
    fprintf(fp_Tape5, "%lf %lf %lf %lf %lf %lf 0\n", initial_height, terminal_height, initial_zenith, path_length, Earth_centre_angle, Earth_radius);//输入第三行数据
    fprintf(fp_Tape5, "%lf %lf %lf\n", start_wave_num, end_wave_num, gap_wave_num);//输入第四行数据
    fprintf(fp_Tape5, "0");//输入第五行数据

    fclose(fp_Tape5);

    rename("Tape5.dat", "Tape5");//将生成的Tape5.dat文件转换为lowtran可读文件

    int judgement_lowtran = 0;
    judgement_lowtran = system("start Lowtran.exe");//运行lowtran

    if (judgement_lowtran != 0)
    {
        printf("错误：无法打开Lowtran.exe\n");
        exit(0);
    }

    Sleep(1000);
    SetCursorPos(run_x_coordinate, run_y_coordinate);//模拟Lowtran运行过程
    mouse_event(MOUSEEVENTF_LEFTDOWN, run_x_coordinate, run_y_coordinate, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, run_x_coordinate, run_y_coordinate, 0, 0);
    Sleep(1000);

    system("taskkill /IM lowtran.exe /F");//关闭lowtran
}

void get_data_transmit(double data_lowtran_transmit[][14])//本函数仅适用于0.3~30微米波段，基于lowtran输出的数据，批量读取并存储大气透过率数据//本函数测试通过
{
    FILE* file_lowtran;
    char line[500];//定义TAPE6某行数据字符串
    int line_number = 0;//指定标识所在行数
    int found_data = 0;//判断标识符是否匹配
    int count = 0;//所在有效数据行，有效数据个数
    int line_number_valid = 0;//读取第一行有效数据，所在行数
    int line_number_judgement = 0;//用于判断是否在读取有效数据

    file_lowtran = fopen("TAPE6", "r");//打开TAPE6文件
    if (file_lowtran == NULL)
    {
        printf("错误：无法打开TAPE6文件\n");
        exit(0);
    }

    line_number = 0;

    while (fgets(line, 500, file_lowtran) != NULL)
    {
        line_number++;
        if (strstr(line, "FREQ WAVELENGTH  TOTAL     H2O     CO2+     OZONE    TRACE  N2 CONT  H2O CONT MOL SCAT  AER-HYD  HNO3    AER-HYD  INTEGRATED") != NULL)//搜索并匹配TAPE6大气透过率数据指定标识
        {
            found_data = 1;
            break;
        }
    }

    line_number_valid = 0;

    if (found_data)
    {
        line_number_valid = line_number + 3;//查询到有效数据所在行
    }
    else
    {
        printf("错误：未找到指定字符串\n");
        exit(0);
    }
    fclose(file_lowtran);

    file_lowtran = fopen("TAPE6", "r");
    if (file_lowtran == NULL)
    {
        printf("错误：无法打开TAPE6文件\n");
        exit(0);
    }

    line_number = 0;
    count = 0;
    line_number_judgement = 0;
    char* token_1;

    for (int i = 0; i < 132; i++)
    {
        for (int line_number_recycle = 0; line_number_recycle < 50; line_number_recycle++)
        {
            line_number_judgement = line_number_valid + line_number_recycle + (50 + 4) * i;//读取第各段数据

            while (fgets(line, 500, file_lowtran) != NULL)
            {
                line_number++;
                if (line_number == line_number_judgement)
                {

                    token_1 = strtok(line, " ");//使用strtok函数分割字符串

                    count = 0;//重置count，否则无法进入下面的子循环

                    while (token_1 != NULL && count < 14)
                    {
                        sscanf(token_1, "%lf", &data_lowtran_transmit[line_number_recycle + 50 * i][count]);//使用sscanf函数将字符串转换为浮点型数据
                        count++;

                        token_1 = strtok(NULL, " ");//获取下一个token，数据间隔符为空格“”
                    }

                    break;
                }
            }

        }

    }

    fclose(file_lowtran);//关闭TAPE6文件

    if (data_lowtran_transmit[0][0] != 330 || data_lowtran_transmit[6599][0] != 33325)//检验0.3~30微米波段数据是否存储规范
    {
        printf("错误：大气透过率数据存储失败\n");
        exit(0);
    }
}

void output_data_transmit(double wavelength_start, double wavelength_end, double data_lowtran_transmit[][14])//本函数用于输出获取的大气透过率数据//本函数测试通过
{
    double distance_wave[6600][2];//定义TAPE6数据对应波长与输入起始、结束波长对应距离

    for (int i = 0; i < 6600; i++)
    {
        distance_wave[i][0] = i;
        distance_wave[i][1] = sqrt((wavelength_start - data_lowtran_transmit[i][1]) * (wavelength_start - data_lowtran_transmit[i][1]));//与起始波长的距离
    }

    int first_num_start = 0;
    double first_start = 9999999999999;//将数值规定为极大数

    for (int i = 0; i < 6600; i++)
    {
        if (distance_wave[i][1] < first_start)
        {
            first_start = distance_wave[i][1];
            first_num_start = i;
        }
    }

    int wavelength_start_num = first_num_start;//找到TAPE6对应起始波长

    for (int i = 0; i < 6600; i++)
    {
        distance_wave[i][0] = i;
        distance_wave[i][1] = sqrt((wavelength_end - data_lowtran_transmit[i][1]) * (wavelength_end - data_lowtran_transmit[i][1]));//与起始波长的距离
    }

    int first_num_end = 0;
    double first_end = 9999999999999;//将数值规定为极大数

    for (int i = 0; i < 6600; i++)
    {
        if (distance_wave[i][1] < first_end)
        {
            first_end = distance_wave[i][1];
            first_num_end = i;
        }
    }

    int wavelength_end_num = first_num_end;//找到TAPE6对应结束波长

    FILE* fp_output_transmit;
    char output_filename[100];

    sprintf(output_filename, "Transmit.dat");//生成文件名
    fp_output_transmit = fopen(output_filename, "w");

    for (int i = 0; i <= 6600; i++)
    {
        if ((i <= wavelength_start_num) && (i >= wavelength_end_num))
        {
            fprintf(fp_output_transmit, "%.4lf %.4lf\n", data_lowtran_transmit[i][1], data_lowtran_transmit[i][2]);
        }
    }

    fclose(fp_output_transmit);

    printf("提示：大气透过率数据已成功输出!\n");
}

int main()
{
    double detect_distance_2 = 0;//定义路径长度，单位m
    double detect_zenith_angle_2 = 0;//定义观测天顶角，单位°
    double target_height = 0;//定义目标初始高度，单位m

    double wavelength_start = 3;
    double wavelength_end = 5;

    printf("请输入目标初始高度：");
    scanf("%lf", &target_height);
    printf("请输入观测天顶角：");
    scanf("%lf", &detect_zenith_angle_2);
    printf("请输入路径长度：");
    scanf("%lf", &detect_distance_2);

    run_lowtran_transmit(detect_distance_2, detect_zenith_angle_2, target_height);
    get_data_transmit(data_lowtran_transmit);
    output_data_transmit(wavelength_start, wavelength_end, data_lowtran_transmit);

    return 0;
}