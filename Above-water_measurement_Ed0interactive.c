/**************************************************************************//**
 * The program is based on demo-averaging.c
 * 
 * @file    demo-averaging.c
 * @date    29-Dec-2014
 * @author  Ocean Optics, Inc.
 * @brief   Demonstrate how to perform multi-scan averaging.
 *
 * LICENSE:
 *
 * SeaBreeze Copyright (C) 2014, Ocean Optics Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h> // For lseek
#include <sys/time.h>
#include <sys/stat.h> //For open
#include <fcntl.h> //For oflags in open
#include <unistd.h> //For close
#include <errno.h>
#include "api/SeaBreezeWrapper.h"
#include "util.h"
#include "api/seabreezeapi/SeaBreezeAPI.h"


#define MAX_LABEL_SIZE     15


int main(int argc, char **argv)
{
	// Measure the time for it takes to run script until end point
	//clock_t t;
	//t = clock();
	
    int error = 0;
    printf("Output from %s \n", argv[0]);

    ////////////////////////////////////////////////////////////////////////////
    // open spectrometer and check if pressure sensor is connected
    ////////////////////////////////////////////////////////////////////////////
	
	// STS-VIS
	
    int specIndex = 0;
    if(seabreeze_open_spectrometer(specIndex, &error))
    {
		// Creates error log file
		int ed = open("/home/pi/projects/Ed0+_raw_data/Error_log.txt", O_CREAT | O_RDWR , 0644); // 
		dprintf(ed, "The spectrometer is not found\n");      
        printf("The spectrometer is not found\n");
        close(ed);
        exit(1);
    }

	
    ////////////////////////////////////////////////////////////////////////////
    // describe the unit we're testing
    ////////////////////////////////////////////////////////////////////////////

    char type[MAX_LABEL_SIZE + 1];
    seabreeze_get_model(specIndex, &error, type, sizeof(type));

    int pixels = seabreeze_get_formatted_spectrum_length(specIndex, &error);

    // printf("Testing %s with %d pixels (index 0)\n", type, pixels);
    
    //////////////////////////////////////////////////////////////////////////////
    //// set measurement parameters CAN CHANGE TO INT??
    //////////////////////////////////////////////////////////////////////////////
    
    // Enter max measurements/stations/files to be created
    unsigned max_stations = 1000;
    
    // Enter integration time (millisec). This is how much time a pixel is "open" it have to be low enough for the device not to saturate. 1000 millisec = 1 sec
    unsigned integ_time_millis = 10; // 2000 ms is good for inside on table with lamp. 220 ms for irradiance/radiance calibration.

	// Enter scans to average
    unsigned scans_to_average = 3;
    
    // Enter maximum number of measurements
    int iterations = 25; // 50
    
    // Enter the time between measurements in seconds
	int size_time_step = 15;
	
	printf("The above-water Ed(0+) will be measured for %d minutes\n", iterations*size_time_step/60);
	
	
    ////////////////////////////////////////////////////////////////////////////
    // configure arrays
    ////////////////////////////////////////////////////////////////////////////
	   
    seabreeze_set_integration_time_microsec(specIndex, &error, integ_time_millis * 1000);
    
    double matrix[pixels][iterations];
    double data[pixels][2+iterations];
    memset(matrix, 0, pixels * (iterations) * sizeof(double));   // Creates an empty matrix with [pixels] rows and [iterations] columns
    memset(data, 0, pixels * (2+iterations) * sizeof(double)); // Creates an empty matrix for storing data to print
    double *spectrum    = (double*) malloc(pixels * sizeof(double));
    double *average     = (double*) malloc(pixels * sizeof(double));
    double *wavelengths = (double*) malloc(pixels * sizeof(double));
    
	
    time_t current_time;
    char* time_string;
    char* times [iterations]; //number of times
    
    for (int i = 0; i < iterations; i++) {
		times[i]        = (char*)   malloc(sizeof(char)*30); // Allocate memory for all times, depths, pressures and temps.
	}
	
    seabreeze_get_wavelengths(specIndex, &error, wavelengths, pixels);
    

    ////////////////////////////////////////////////////////////////////////////
    // running the loop for the length of the experiment		
    ////////////////////////////////////////////////////////////////////////////
    
    int loop_count = 0;
    
	for (unsigned u = 0; u < iterations; u++) { // Measurement loop
		printf("Loop count: %d\n", loop_count);
		printf("Taking spectral measurement.\n");
		memset(average, 0, pixels * sizeof(double));
		for (int i = 0; i < scans_to_average; i++) {
			memset(spectrum, 0, pixels * sizeof(double));
			seabreeze_get_formatted_spectrum(specIndex, &error, spectrum, pixels);
			for (unsigned j = 0; j < pixels; j++)
				average[j] += spectrum[j];
		}
		current_time = time(NULL); // Obtain current time
		time_string = ctime(&current_time); // Convert to local time format
		time_string[strcspn(time_string, "\n")] = '\0'; //get rid of new line character that the time strin gcontains by default (\n)
    
		for (unsigned i = 0; i < pixels; i++) {  // Save average spectrum in a matrix
			average[i] /= scans_to_average;
			matrix[i][u] = average[i];
			//printf("Count: %lf\n", matrix[i][u]);
			
		}
		
		strcpy(times[u],time_string); // Copy time_string to times[i]
		
		loop_count += 1; // Adds 1 every loop
		printf("Sleeps for %d sec.\n", size_time_step);
		sleep(size_time_step);
	}
	printf("Done with the acquisition.\n");
	

    ////////////////////////////////////////////////////////////////////////////
    // create and write averaged spectra to .txt with increasing numbers
    ////////////////////////////////////////////////////////////////////////////
    
    printf("Writing data to file...\n");
    
    // Arrange a new data matrix ready for dprintf to .txt
	for (unsigned k = 0; k < pixels; k++){
		data[k][0] = k;
		data[k][1] = wavelengths[k];
		for (unsigned u = 2; u < (2+iterations); u++) {
			data[k][u] = matrix[k][u-2];
		}
	}
        
	// Check if file exist, if no then create file and write. If yes then move to next number (iteration) and create file and write. Also creates the filenames
	char namepath[50]; // Bytes to store string
	
	for (int i = 0; i < max_stations; i++) { // Will only loop once
		if (i == max_stations) {
			// Creates error log file if the for-loop reached its last iteration
			int ed = open("/home/pi/projects/Ed0+_raw_data/Error_log.txt", O_CREAT | O_RDWR, 0644); // 
			dprintf(ed, "The datafile writing loop reached its last iteratiuon and will not be able to create any new data files\n");
		}
		snprintf(namepath,50,"/home/pi/projects/Ed0+_raw_data/DataEd0+%d.txt",i);
		int fd = open(namepath, O_CREAT | O_RDWR | O_EXCL, 0644); // Check for existance and create the file if it does not exists
		
		if (fd < 0) { // WRITES DATAX.txt FILES
			// it exists
			continue; // skips the current iteration of the for-loop and continues with the next for-loop iteration
		} else {
			//it does not exist, it is created and then:
			dprintf(fd, "Ed0+ STS-VIS Miniature Spectrometer settings\n");
			dprintf(fd, "Integration time (ms): %d\nNumber of averaged scans: %d\nNumber of spectral measurements: %d\n", integ_time_millis, scans_to_average, loop_count);
				for (int u= 0; u < iterations; u++){
					dprintf(fd, "Time at every iteration: %s\n", times[u]);
				}
			dprintf(fd, "Pixel, Wavelength, Intensities (counts) every %d seconds\n", size_time_step);
			for (unsigned k = 0; k < pixels; k++){
				for (unsigned u = 0; u < (2+iterations); u++) {
						dprintf(fd, "%lf,", data[k][u]);
				}
				dprintf(fd, "\n");
			}
			close (fd);
			break;		
		 }
	}

    // Writes all measurements in depth for the same station in one file, but with increasing column

    printf("End.\n");
    return (0);


    ////////////////////////////////////////////////////////////////////////////
    // cleanup
    ////////////////////////////////////////////////////////////////////////////
    
    free(times);
    free(average);
    free(spectrum);
    seabreeze_close_spectrometer(specIndex, &error);
}
