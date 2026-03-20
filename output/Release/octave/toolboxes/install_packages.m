%------------------------------------------------------------------------------
%------------------------------------------------------------------------------
% ARIA Sensing - 2026
% This is a simple program to install all required packages
% for the ARIA-RDK environment
%------------------------------------------------------------------------------
%------------------------------------------------------------------------------
% Note: this m file must be executed from the "Release/output/octave/toolboxes"
% folder
clear("all");
clear("variables");

initial_dir = pwd();
install_dir = pwd();
package_dir = pwd();

% Set to 1 if Windows, 0 if Linux
windows = 1;

if (windows==1)
  cd("tmp");
  tdir = tempdir();
  setenv("TMPDIR",tdir);
  
  % Go to the folder where archives are stored
  cd(package_dir);
  % 
  pkg("prefix",install_dir);
  
  % Install control
  printf("Installing control toolbox\n");
  pkg("install","control-4.2.1.zip");
  printf("done\n");
  
  % Install signal
  printf("Installing sigmal toolbox\n");
  pkg("install","signal-1.4.7.zip");
  printf("done\n");  

  % Install image
  printf("Installing image\n");
  pkg("install","image-2.20.0.zip");
  printf("done\n");    

  % Install communications
  printf("Installing communications\n");
  pkg("install","communications-1.2.7.zip");
  printf("done\n");    
  
  % Install ARIA UWB Toolbox
  printf("Installing ARIA UWB toolbox \n");
  pkg("install","ARIA_uwb_toolbox-main.zip");
  printf("done\n");    
  
  % Install ARIA UWB Toolbox
  printf("Installing LTFAT \n");
  pkg("install","ltfat-2.6.0-of.zip");
  printf("done\n");    
else
  cd("tmp");
  tdir = tempdir();
  setenv("TMPDIR",tdir);
  
  % Go to the folder where archives are stored
  cd(package_dir);
  % 
  pkg("prefix",install_dir);
  
  % Install control
  printf("Installing control toolbox\n");
  pkg("install","control-4.2.1.tar.gz");
  printf("done\n");
  
  % Install signal
  printf("Installing sigmal toolbox\n");
  pkg("install","signal-1.4.7.tar.gz");
  printf("done\n");  
  
 
  % Install image
  printf("Installing image\n");
  pkg("install","image-2.20.0.tar.gz");
  printf("done\n");    

  % Install communications
  printf("Installing communications\n");
  pkg("install","communications-1.2.7.tar.gz");
  printf("done\n");    
  
  % Install ARIA UWB Toolbox
  printf("Installing ARIA UWB toolbox \n");
  pkg("install","ARIA_uwb_toolbox-main.tar.gz");
  printf("done\n");    
  
endif;
 
cd(initial_dir);