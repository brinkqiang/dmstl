#ifndef GDEFINE_H_D
#define GDEFINE_H_D

#define g_key_path "."			//程序路径，为了ftok算共享内存唯一ID值
#define g_key_base "0"			//为了ftok算共享内存唯一ID值给一个base，免得和同路径下其他同样要用共享内存的程序算出来的ID冲突
#define g_chunck_num "1"	  	//总申请共享内存块的个数，1个就是一块独立的共享内存块
#define g_chunck_size "33554432"  	//块的页面数,为1至4096，则实际申请到的共享内存大小为4K(一页),4097到8192，则实际申请到的共享内存大小为8K(两页)，依此类推33554432是32M

#endif
