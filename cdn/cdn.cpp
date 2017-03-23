#include "deploy.h"
#include "lib_io.h"
#include "lib_time.h"
#include "stdio.h"

#define LOCAL 23

int main(int argc, char *argv[])
{
    print_time("Begin");
    char *topo[MAX_EDGE_NUM];
    int line_num;

#ifdef LOCAL
    const char *const topo_file = "/Users/sooglejay/StudyC/projectXcode/HW/HW/cases/case0.txt";
    const char *const result_file = "/Users/sooglejay/StudyC/projectXcode/HW/HW/output.txt";

#else
    const char * const topo_file = argv[1];
    const char * const result_file = argv[2];
#endif


    line_num = read_file(topo, MAX_EDGE_NUM, topo_file);

    printf("line num is :%d \n", line_num);
    if (line_num == 0)
    {
        printf("Please input valid topo file.\n");
        return -1;
    }


//    deploy_server(topo, line_num, result_file);
    deploy_server_ex(topo_file, result_file);

    release_buff(topo, line_num);

    print_time("End");

	return 0;
}

