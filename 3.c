#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
	unsigned char r;
	unsigned char g;
	unsigned char b;
} pixel;

pixel* img, *img_result;

void flip_image(int width, int height, FILE* fp_w) {
	int r, c, t;
	img_result = (pixel*) malloc (sizeof(pixel)*width*height);
	
	for(r=0; r<height; r++) {
		for(c=0; c<width/2; c++) {
			t=width-c-1;
			img_result[r*width+c]=img[r*width+t];
			img_result[r*width+t]=img[r*width+c];			
		}	
		img_result[r*width+c]=img[r*width+c];
	}	
	pixel *p;
	for(r=0; r<height; r++) {
		for(c=0; c<width; c++) {
			p = img_result+r*width+c;
			fprintf(fp_w,"%c%c%c",p->r, p->g, p->b);
		}
	}

	free(img_result);
	fclose(fp_w);
}

void grayscale_image(int width, int height, FILE* fp_w)
{
	int r, c;
	unsigned int tmp;
	unsigned char p;
	
	img_result = (pixel*) malloc (sizeof(pixel)*width*height);
	
	for(r=0; r<height; r++) {
		for(c=0; c<width; c++) {
			tmp = img[r*width+c].r + img[r*width+c].g + img[r*width+c].b; 
			p = (unsigned char) (tmp/3);
			img_result[r*width+c].r=p;
			img_result[r*width+c].g=p;
			img_result[r*width+c].b=p;
		}	
	}	
	pixel *pt;
	for(r=0; r<height; r++) {
		for(c=0; c<width; c++) {
			pt = img_result+r*width+c;
			fprintf(fp_w,"%c%c%c",pt->r, pt->g, pt->b);
		}
	}

	free(img_result);
	fclose(fp_w);
}
double getRValue(int row, int col, int width, int height) {
	if (row < 0 || col < 0 || row >= height || col >= width)
		return 0;
	return img[row * width + col].r;	
}
double getGValue(int row, int col, int width, int height) {
	if (row < 0 || col < 0 || row >= height || col >= width)
		return 0;
	return img[row * width + col].g;	
}
double getBValue(int row, int col, int width, int height) {
	if (row < 0 || col < 0 || row >= height || col >= width)
		return 0;
	return img[row * width + col].b;	
}
void mean_filtering_image(int width, int height, FILE* fp_w)
{
	int r, c, idx, k, l, w;
	unsigned int tmp;
	unsigned char p;
	double r_sum,g_sum,b_sum;
	
	img_result = (pixel*) malloc (sizeof(pixel)*width*height);
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
			img_result[r*width+c].r=(unsigned char)(r_sum/9);
			img_result[r*width+c].g=(unsigned char)(g_sum/9);;
			img_result[r*width+c].b=(unsigned char)(b_sum/9);;
		}	
	}	
	pixel *pt;
	for(r=0; r<height; r++) {
		for(c=0; c<width; c++) {
			pt = img_result+r*width+c;
			fprintf(fp_w,"%c%c%c",pt->r, pt->g, pt->b);
		}
	}

	free(img_result);
	fclose(fp_w);	
}
int main(int argc, char* argv[])
{
	clock_t start, end;
	FILE *fp_r, *fp_w; 
	char magic[5];
	int width, height, maxval;
	int r, c; 
	pixel *p;
	
	if (argc<3) {
		printf("arg : image_path process_mod(0,1,2)\n");
		return -1;
	}
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

	// read image 
	img = (pixel*)malloc(sizeof(pixel) * width * height); 
	for (r=0; r<height; r++) {
		for ( c=0; c<width; c++) {
			p = img + (r*width+c);
			fscanf(fp_r,"%c%c%c",&p->r, &p->g, &p->b);		
//			if( r==0 && c==0)
//				printf("%x%x%x",p->r,p->g, p->b);
		}
	}
	fclose(fp_r);

	start = clock();

	switch (atoi(argv[2])){
		case 0:
			// flip image horizontal
			printf("flip image horizontal\n");
			fp_w = fopen("img_flipped.ppm", "w");
			fprintf(fp_w,"%s\n%d\n%d\n%d\n", magic, width, height, maxval);
			flip_image(width, height, fp_w);
			break;

		case 1:
			// reduce to grayscale
			printf("convert to grayscale\n");
			fp_w = fopen("img_gray.ppm","w");
			fprintf(fp_w,"%s\n%d\n%d\n%d\n", magic, width, height, maxval);
			grayscale_image(width, height, fp_w);
			break;

		case 2:	
			// mean filtering
			printf("mean filtering\n");
			fp_w = fopen("img_filter.ppm","w");
			fprintf(fp_w,"%s\n%d\n%d\n%d\n", magic, width, height, maxval);
			mean_filtering_image(width, height, fp_w);
			break;
	}
	end = clock();

	printf("Elapsed Time: %lf second\n", (double)(end - start)/CLOCKS_PER_SEC);
	return 0;
}
