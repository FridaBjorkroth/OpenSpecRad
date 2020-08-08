/**************************************************************************//**
 * Program based on demo-averaging.c
 * Frida Bjorkroth, 08-Aug-2020
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
#include "python2.7/Python.h"


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
		int ed = open("/home/pi/projects/Luz_raw_data/Error_log.txt", O_CREAT | O_RDWR , 0644); // 
		dprintf(ed, "The spectrometer is not found\n");      
        printf("The spectrometer is not found\n");
        close(ed);
        exit(1);
    }

	// Bar30
	// Creates error log file if python script fails and then ends the C-script
	Py_Initialize();
	PyObject* PyScript = PyFile_FromString("/home/pi/ms5837-python/DepthTempMeasurement.py", "r");
	PyRun_SimpleFileEx(PyFile_AsFile(PyScript), "/home/pi/ms5837-python/DepthTempMeasurement.py", 1);
	Py_Finalize();
	

	
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
    unsigned integ_time_millis;// = 10; // 2000 ms is good for inside on table with lamp. 220 ms for irradiance/radiance calibration.
	printf("Input integration time (ms): \n");
    fflush(stdout);
	scanf("%u", &integ_time_millis);
	
	// Enter scans to average
    unsigned scans_to_average = 3;
    
    // Enter the maximun depth (m) for the measurement
    double max_depth; // = 20;
    printf("Input max depth (m): \n");
    fflush(stdout);
	scanf("%lf", &max_depth);
    
    // Enter the depth that triggers the measurement to start
	double min_depth; // = 0.5;
	printf("Input min depth (m): \n");
    fflush(stdout);
	scanf("%lf", &min_depth);
	
	// Enter the depth resolution. Measurement at every 0.1 meters.
	double depth_res; // = 0.1;
	printf("Input depth resolution (m): \n");
    fflush(stdout);
	scanf("%lf", &depth_res);
	
	// Enter the depth precision. Measurement at every 0.1 meters.
	double depth_prec; // = 0.1;
	printf("Input depth precision (m): \n");
    fflush(stdout);
	scanf("%lf", &depth_prec);
    
    // Enter number of spectral measurement. Every 0.1 meter until 20 meters, starting at 0.5 m gives 196 measurements, but loop also runs at loop_count=0, so 195.
    // Total numer of columns with intensities is 196, so write max_meas+1 when used for sizeing
    int num_meas = (max_depth-min_depth)/depth_res;
    
    int col = num_meas+1; // Number of columns to fit the 0-195 (i.ie 196 measurements)
		
	// Enter how many seconds the insturment will wait to check if the in-water instrument is at the requested depth.
	int nap; // = 1;
	printf("Input nap time (s) for reaching requested depth: \n");
    fflush(stdout);
	scanf("%d", &nap);
	
	// Enter how many times it will retry to reach the requested depth before ending
	int fail_retries; // = 25;
	printf("Input no. retries for reaching reqested depth: \n");
    fflush(stdout);
	scanf("%d", &fail_retries);
	
	
    ////////////////////////////////////////////////////////////////////////////
    // configure arrays
    ////////////////////////////////////////////////////////////////////////////
	   
    seabreeze_set_integration_time_microsec(specIndex, &error, integ_time_millis * 1000);
    
    double matrix[pixels][col];
    double data[pixels][2+col];
    memset(matrix, 0, pixels * (col) * sizeof(double));   // Creates an empty matrix with [pixels] rows and [iterations] columns
    memset(data, 0, pixels * (2+col) * sizeof(double)); // Creates an empty matrix for storing data to print
    double *spectrum    = (double*) malloc(pixels * sizeof(double));
    double *average     = (double*) malloc(pixels * sizeof(double));
    double *wavelengths = (double*) malloc(pixels * sizeof(double));
    
    double depth; //       = (double*) malloc(sizeof(double)); // pointer variable declaration
	double pressure; //    = (double*) malloc(sizeof(double));
	double temperature; // = (double*) malloc(sizeof(double));
	
	double* depths       [col]; // pointer variable declaration
	double* pressures    [col];
	double* temperatures [col];
	memset(depths, 0, (col) * sizeof(double));
	memset(pressures, 0, (col) * sizeof(double));
	memset(temperatures, 0, (col) * sizeof(double));
	
    time_t current_time;
    char* time_string;
    char* times [col]; //number of times
    
    for (int i = 0; i < col; i++) {
		times[i]        = (char*)   malloc(sizeof(char)*30); // Allocate memory for all times, depths, pressures and temps.
		depths[i]       = (double*) malloc(sizeof(double)*15); // 4 elements in each depth
		pressures[i]    = (double*) malloc(sizeof(double)*15);
		temperatures[i] = (double*) malloc(sizeof(double)*15);
	}
	
    seabreeze_get_wavelengths(specIndex, &error, wavelengths, pixels);
    

    ////////////////////////////////////////////////////////////////////////////
    // Make a spectrometer measurement if the depth is equal to or more than 0.5 meters (at Edz spectrometer entrance).
    // For Luz, the triggering depth is 0.5 meters at Luz spectrometer entrance. Then, take measurement every 0.1 meters		
    ////////////////////////////////////////////////////////////////////////////

	int loop_count = 0;
	int fail_count = 0;
	
	// Start the acquisition when the in-water instrument is underwater, at decided depth
	while (1){ //loop_count <= max_meas+1
		printf("Loop count: %d\n", loop_count);
		// First measure the depth
		Py_Initialize();
		PyObject* PyScript = PyFile_FromString("/home/pi/ms5837-python/DepthTempMeasurement.py", "r");
		PyRun_SimpleFileEx(PyFile_AsFile(PyScript), "/home/pi/ms5837-python/DepthTempMeasurement.py", 1);
		Py_Finalize();
		FILE * pyfd;
		pyfd = fopen("/home/pi/ms5837-python/PyOutput.txt", "r");
			if (pyfd == NULL){
				printf("Could not open file PyOutput.txt to read the Bar30 data\n");
				return 1;
			}
	    fscanf(pyfd, "%lf %lf %lf", &depth, &pressure, &temperature); // Read the data from the file output by the Bar30 Python script
		fclose(pyfd);
		
		printf("Requested depth: %lf\n", min_depth+depth_res*loop_count);
		
		//depth = min_depth+depth_res*loop_count; // REMOVE AFTER TEST
		
		// Clock time end point, can be moved to test
		//t = clock() - t;
		//double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds
		//printf("It took %f seconds to execute the code\n", time_taken);
		
		if (loop_count > num_meas){ // Ends the measurement after 195 loops, breaking the while-loop
			  break;
			  
			} else if (depth >= min_depth-depth_prec+depth_res*loop_count && depth <= min_depth+depth_prec+depth_res*loop_count){ 
			// May be a problem that the depth must EXACTLY be 0.5, 0.6, 0.6 etc to trgger spectral measurement. (depth == min_depth+depth_res*loop_count) 
			// I could write: Do one measurement while within this range, i.e +- 2.5 cm (depth >= min_depth-depth_prec+depth_res*loop_count && depth <= min_depth+depth_prec+depth_res*loop_count)
			
			//printf("Min depth: %lf\n", min_depth);
			//printf("Depth res: %lf\n", depth_res);
			//printf("Loop count: %d\n", loop_count);
			
			// Take a first spectal measurement
			printf("Taking spectral measurement at depth %lf\n", depth);
			memset(average, 0, pixels * sizeof(double));
			for (int i = 0; i < scans_to_average; i++) {
				memset(spectrum, 0, pixels * sizeof(double));
				seabreeze_get_formatted_spectrum(specIndex, &error, spectrum, pixels);
					for (unsigned j = 0; j < pixels; j++)
					average[j] += spectrum[j];
					}
				current_time = time(NULL); // Obtain current time
				//printf("%ld\n", time(NULL));
				time_string = ctime(&current_time); // Convert to local time format
				time_string[strcspn(time_string, "\n")] = '\0'; //get rid of new line character that the time strin gcontains by default (\n)
    
				for (unsigned i = 0; i < pixels; i++) {  // Save average spectrum in a matrix
					average[i] /= scans_to_average;
					matrix[i][loop_count] = average[i];
					//printf("Count: %lf\n", matrix[i][loop_count]);
					}
			strcpy(times       [loop_count], time_string); // Copy time_string to times[i]
			memcpy(depths      [loop_count], &depth,sizeof(double));
			memcpy(temperatures[loop_count], &temperature,sizeof(double));
			memcpy(pressures   [loop_count], &pressure,sizeof(double));
			
			loop_count += 1; // Adds 1 every loop that resulted in a spectral measurement, so loop_count = 1 + loop_count
			
			} 
			else if (fail_count == fail_retries-1){ // Ends the measurement after [fail_retries-1] retries moves on.
				printf("Did not reach the requested depth.\n");
				// Creates error log file
				int ed = open("/home/pi/projects/Luz_raw_data/Error_log.txt", O_CREAT | O_RDWR , 0644); // 
				dprintf(ed, "Did not reach the requested depth.\n");
				close(ed); 
				break;
				
			} else {
				printf("Current depth is %lf m. Put the in-water instrument at %lf m to start the acquisition.\n", depth, min_depth+depth_res*loop_count);
				fail_count += 1;
				sleep(nap);
				continue;
					}
	}
	printf("Done with the acquisition.\n");

    ////////////////////////////////////////////////////////////////////////////
    // create and write averaged spectra to .txt with increasing numbers
    ////////////////////////////////////////////////////////////////////////////
    
    printf("Writing data to file...\n");
    
    // Arrange a new data matrix ready for dprintf to .txt
	for (unsigned k = 0; k < pixels; k++){ // k = row
		data[k][0] = k;
		data[k][1] = wavelengths[k];
		for (unsigned u = 2; u < (2+col); u++) {
			data[k][u] = matrix[k][u-2];
		}
	}
        
	// Check if file exist, if no then create file and write. If yes then move to next number (iteration) and create file and write. Also creates the filenames
	char namepath[50]; // Bytes to store string
	
	
	
	for (int i = 0; i < max_stations; i++) { // Will only loop once
		if (i == max_stations) {
			// Creates error log file if the for-loop reached its last iteration
			int ed = open("/home/pi/projects/Luz_raw_data/Error_log.txt", O_CREAT | O_RDWR, 0644); // 
			dprintf(ed, "The datafile writing loop reached its last iteratiuon and will not be able to create any new data files\n");
			close(ed);
			exit(1);
		}
		snprintf(namepath,50,"/home/pi/projects/Luz_raw_data/DataLuz%d.txt",i);
		int fd = open(namepath, O_CREAT | O_RDWR | O_EXCL, 0644); // Check for existance and create the file if it does not exists
		
		if (fd < 0) { // WRITES DATAX.txt FILES
			// it exists
			continue; // skips the current iteration of the for-loop and continues with the next for-loop iteration
		} else {
			//it does not exist, it is created and then:
			dprintf(fd, "Luz STS-VIS Miniature Spectrometer settings\n");
			dprintf(fd, "Integration time (ms): %d\nNumber of averaged scans: %d\nNumber of spectral measurements: %d\nDepth resolution: %lf\nDepth precision: %lf\nNap (s): %d\nFail retries: %d\n", integ_time_millis, scans_to_average, loop_count, depth_res, depth_prec, nap, fail_retries);
			dprintf(fd, "Time, Depth (m), Pressure (mbar), Temperature (C)\n");
				for (int u = 0; u < loop_count; u++){
					dprintf(fd, "%s, %lf, %lf, %lf\n", times[u], *depths[u], *pressures[u], *temperatures[u]);
				}
			dprintf(fd, "Pixel, Wavelength, Intensities at every depth\n");
			for (unsigned k = 0; k < pixels; k++){
				for (unsigned u = 0; u < (2+loop_count); u++) {
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
    
    free(depths);
    free(pressures);
    free(temperatures);
    free(times);
    free(average);
    free(spectrum);
    seabreeze_close_spectrometer(specIndex, &error);
}
