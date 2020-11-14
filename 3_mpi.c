#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} pixel;

pixel* img_input, *img_flip, *img_gray, *img_filter, *img;

void fprint_image(FILE *fp_w, pixel* img_result, int width, int height) {
    int r, c;
    pixel *p;

    for(r=0; r<height; r++) {
        for(c=0; c<width; c++) {
            p = img_result+r*width+c;
            fprintf(fp_w,"%c%c%c",p->r, p->g, p->b);
        }
    }
} 
void flip_image(int width, int height) {
    int r, c, t;
    img_flip = (pixel*) malloc (sizeof(pixel)*width*height);

    for(r=0; r<height; r++) {
        for(c=0; c<width/2; c++) {
            t=width-c-1;
            img_flip[r*width+c]=img[r*width+t];
            img_flip[r*width+t]=img[r*width+c];			
        }	
        img_flip[r*width+c]=img[r*width+c];
    }	
}

void grayscale_image(int width, int height)
{
    int r, c;
    unsigned int tmp;
    unsigned char p;

    img_gray = (pixel*) malloc (sizeof(pixel)*width*height);

    for(r=0; r<height; r++) {
        for(c=0; c<width; c++) {
            tmp = img[r*width+c].r + img[r*width+c].g + img[r*width+c].b; 
            p = (unsigned char) (tmp/3);
            img_gray[r*width+c].r=p;
            img_gray[r*width+c].g=p;
            img_gray[r*width+c].b=p;
        }	
    }	
}
double getRValue(int row, int col, int width, int height) {
	if (row < 0 ) row = 0;
	if (row >= height) row = height - 1;
	if (col < 0 ) col = 0;
	if (col >= width) col = width - 1;
    return img[row * width + col].r;	
}
double getGValue(int row, int col, int width, int height) {
	if (row < 0 ) row = 0;
	if (row >= height) row = height - 1;
	if (col < 0 ) col = 0;
	if (col >= width) col = width - 1;
    return img[row * width + col].g;	
}
double getBValue(int row, int col, int width, int height) {
	if (row < 0 ) row = 0;
	if (row >= height) row = height - 1;
	if (col < 0 ) col = 0;
	if (col >= width) col = width - 1;
    return img[row * width + col].b;	
}
void mean_filtering_image(int width, int height)
{
    int r, c, idx, k, l, w;
    unsigned int tmp;
    unsigned char p;
    double r_sum,g_sum,b_sum;

    img_filter = (pixel*) malloc (sizeof(pixel)*width*height);
    w = 1;

    for(r=0; r<height; r++) {
        for(c=0; c<width; c++) {
            idx =r*width+c;
            r_sum=0; g_sum=0; b_sum=0;
            for (k = -w; k <= w; k++) { 
                for (l = -w; l <= w; l++) {
                    r_sum += getRValue(r+k, c+l, width, height);
                    g_sum += getGValue(r+k, c+l, width, height);
                    b_sum += getBValue(r+k, c+l, width, height);
                }
            } 
            img_filter[r*width+c].r=(unsigned char)(r_sum/9);
            img_filter[r*width+c].g=(unsigned char)(g_sum/9);;
            img_filter[r*width+c].b=(unsigned char)(b_sum/9);;
        }	
    }	
}
int main(int argc, char *argv[])
{
    clock_t start, end;
    FILE *fp_r, *fp_flip, *fp_gray, *fp_filter; 
    char magic[5];
    int width, height, maxval;
    int r, c, n, n_local; 
    pixel* img_flip_out, *img_gray_out, *img_filter_out;
    pixel *p;
    int numprocs, rank, namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Request *reqs; MPI_Status *stats;
    int buf[3];
	int *n_cnt, *n_disp, i;

    if (argc<3) {
        printf("arg : image_path process_mod(0,1,2)\n");
        return -1;
    }

    // MPI Initialize
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(processor_name, &namelen);

    MPI_Datatype Pixeltype;
    MPI_Datatype type[3] = {MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR};
    int blocklen[3] = {1,1,1};
    MPI_Aint disp[3] = {0,1,2};//, off, base;

    MPI_Type_create_struct(3, blocklen, disp, type, &Pixeltype);
    MPI_Type_commit(&Pixeltype);


    if (rank == 0) { /* master */	
        // Open file
        //fp_r = fopen("ppm_example/small/house_1.ppm","r");
        //fp_r = fopen("ppm_example/large/sage_1.ppm","r");
        //fp_r = fopen("ppm_example/Iggy.1024.ppm","r");
        fp_r = fopen(argv[1],"r");
        if (fp_r == NULL) {
            printf("file open error\n");
            return -1;
        }

        // Read ppm file header
        fscanf(fp_r, "%s", magic);
        printf("%s\n", magic);
        fseek(fp_r, 1, SEEK_CUR); 
        if ( fgetc(fp_r) == '#' ) {
            while (fgetc(fp_r) != '\n'); 
        } else {
            fseek(fp_r, -1, SEEK_CUR);
        }
        fscanf(fp_r,"%d\n%d\n%d\n", &width, &height, &maxval);
        printf("%d\n%d\n%d\n", width, height, maxval);
        if(maxval>255) {
            printf("please open 8-bit image file :)\n");
            return 0;
        }

        // send width, height, n_local
        //		if (height < numprocs)	numprocs = height;
        n_local = height / numprocs;


        buf[0]=width; buf[1]=height; buf[2]=n_local;
        MPI_Bcast(buf,3,MPI_INT,0,MPI_COMM_WORLD);

        // read image 
        img_input = (pixel*)malloc(sizeof(pixel) * width * height); 
        for (r=0; r<height; r++) {
            for ( c=0; c<width; c++) {
                p = img_input + (r*width+c);
                fscanf(fp_r,"%c%c%c",&p->r, &p->g, &p->b);		
                //			if( r==0 && c==0)
                //				printf("%x%x%x",p->r,p->g, p->b);
            }
        }
        fclose(fp_r);

        img_flip_out = (pixel*)malloc(sizeof(pixel) * width * height); 
        img_gray_out = (pixel*)malloc(sizeof(pixel) * width * height); 
        img_filter_out = (pixel*)malloc(sizeof(pixel) * width * height); 

		n_cnt = (int*) malloc(sizeof(int) * numprocs);
		n_disp = (int*) malloc(sizeof(int) * numprocs);

		for (i=0; i<numprocs; i++) {
			n_cnt[i]= width * n_local;
			n_disp[i]= width * n_local * i;
			if (i == numprocs-1) {
				n_cnt[i] += width*(height % numprocs);
			}
		}
	} else { /* worker */

        MPI_Bcast(buf,3,MPI_INT,0,MPI_COMM_WORLD);
        width=buf[0]; height=buf[1]; n_local=buf[2];
    }

    if (rank == numprocs-1)
        n_local += height % numprocs;

    start = clock();

    img = (pixel*) malloc(sizeof(pixel) * width * n_local);
    MPI_Scatterv(img_input, n_cnt, n_disp, Pixeltype, img, width * n_local, Pixeltype, 0, MPI_COMM_WORLD);

	
    reqs = (MPI_Request*) malloc(sizeof(MPI_Request));
    stats = (MPI_Status*) malloc(sizeof(MPI_Status));
    switch (atoi(argv[2])){
        case 0:
            // flip image horizontal
            flip_image(width, n_local);
            MPI_Igatherv(img_flip, width * n_local, Pixeltype, img_flip_out, n_cnt, n_disp, Pixeltype, 0, MPI_COMM_WORLD, reqs);
            if (rank == 0) { /* master */
                printf("flip image horizontal\n");
                fp_flip = fopen("img_flipped.ppm", "w");
                fprintf(fp_flip,"%s\n%d\n%d\n%d\n", magic, width, height, maxval);
                MPI_Wait(reqs, stats);
                fprint_image(fp_flip, img_flip_out, width, height);
                fclose(fp_flip);		
            }
            break;

        case 1:
            // reduce to grayscale
            grayscale_image(width, n_local);
            MPI_Igatherv(img_gray, width * n_local, Pixeltype, img_gray_out, n_cnt, n_disp, Pixeltype, 0, MPI_COMM_WORLD, reqs);
            if (rank == 0) { /* master */
                printf("convert to grayscale\n");
                fp_gray = fopen("img_gray.ppm","w");
                fprintf(fp_gray,"%s\n%d\n%d\n%d\n", magic, width, height, maxval);
                MPI_Wait(reqs, stats);
                fprint_image(fp_gray, img_gray_out, width, height);
                fclose(fp_gray);		
            }
            break;

        case 2:	
            // mean filtering
            mean_filtering_image(width, n_local);
            MPI_Igatherv(img_filter, width * n_local, Pixeltype, img_filter_out, n_cnt, n_disp, Pixeltype, 0, MPI_COMM_WORLD, reqs);
            if (rank == 0) { /* master */
                printf("mean filtering\n");
                fp_filter = fopen("img_filter.ppm","w");
                fprintf(fp_filter,"%s\n%d\n%d\n%d\n", magic, width, height, maxval);
                MPI_Wait(reqs, stats);
                fprint_image(fp_filter, img_filter_out, width, height);
                fclose(fp_filter);		
            }
            break;
    }
    end = clock();

    MPI_Finalize();

    if (rank == 0) {

        end = clock();
        printf("Elapsed Time: %lf second\n", (double)(end - start)/CLOCKS_PER_SEC);
        free(img_input);
        free(img_flip_out);
        free(img_gray_out);
        free(img_filter_out);
    }

    free(img);
    free(img_flip);
    free(img_gray);
    free(img_filter);
    return 0;
}
