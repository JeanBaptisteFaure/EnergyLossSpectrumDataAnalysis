# EnergyLossSpectrumDataAnalysis
Data Analysis Software for Energy Loss Spectra

Given text files with at least raw data of energy loss spectra and an energy file, this Software provides a GUI that allows you to analyze your data by:
1. Generating the associated background noise using a linear least squares regresion if you do not provide the background files. 
2. Calibrating the data to the proper energies using the energy files and your chosen energy peaks for the atom or molecule in question.
3. Performing a Non-Linear Least Squares Regression and optimizing it using Chi-Squared values in order to fit a gaussian to the best peak in the data.
4. Performing a Linear Least Squares Regression using the information from step 3 as starting point. 
5. Providing relevant information for the fit and goodness of the fit (i.e. chi-squared of each peak, energies of each peak, and the areas of said peaks etc.), all exported to a CSV file for easy comparison with theoretical models of the molecule or atom in question.  

Must use QT Creator Version 5.9.9 or other version that is compatible with QCustomPlot.

To create an application from these files, first open them in QT Creator and build. Then you can find multiple ways online to make an application from a QT build. 

When running the GUI, there is a detailed instructions tab on how to use the GUI at the top left. Use the example files in the "Data" folder provided to check your understanding. 
