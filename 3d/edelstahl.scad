/*
 * Edelstahl Case
 * Copyright (c) 2026 Lone Dynamics Corporation. All rights reserved.
 *
 * required hardware:
 *  - 4 x M3 x 20mm countersunk bolts
 *  - 4 x M3 nuts
 *
 */

$fn = 100;

board_width = 32;
board_thickness = 1.5;
board_length = 52;
board_height = 1.6;
board_spacing = 2;

wall = 1.5;

top_height = 15.8;
cover_height = 90;
bottom_height = 6;

//translate([wall,wall,0]) msx_board();

translate([0,0,0])
	msx_case_top();

translate([0,0,-6])
	msx_case_bottom();

//translate([6.5,6.5,-5]) color([1,0,0]) cylinder(d=2, h=20);

module msx_board() {
	
	difference() {
		color([0,0.5,0])
			roundedcube(board_width,board_length,board_thickness,5);
		translate([5, 5, -1]) cylinder(d=3.2, h=10);
		translate([5, 52-5, -1]) cylinder(d=3.2, h=10);
		translate([32-5, 5, -1]) cylinder(d=3.2, h=10);
		translate([32-5, 52-5, -1]) cylinder(d=3.2, h=10);
	}	
	
}

module msx_case_top() {
	
	difference() {
			
       union() { 
            color([0.5,0.5,0.5])
            roundedcube(board_width+(wall*2),board_length+(wall*2),top_height,5);
       }
	

		translate([wall, wall, 0]) {
            
			roundedcube(board_width,board_length,11,5);
            
			// USB-C
			translate([32/2-(9.5/2),50,0]) cube([9.5,20,3.5]);            

			// boot button
			translate([32/2-(7.6/2),-5,0]) cube([7.6,20,7]);

            // card socket
            translate([7,26,0]) cube([9.3+0.1, 33.2+0.1, 100] ,center=true);

            color([0.7,0.7,0.7]) {
//                translate([20,12,16]) cube([5,5,1], center=true);
//                translate([20,20,16]) cube([5,5,1], center=true);
//                translate([20,28,16]) cube([5,5,1], center=true);
                translate([16,40,15.5]) cylinder(d=2.5, h=10); // pin 1

            }
            
            translate([5, 5, top_height-2.5]) cylinder(d=5.6, h=4);
            translate([5, 52-5, top_height-2.5]) cylinder(d=5.6, h=4);
            translate([32-5, 5, top_height-2.5]) cylinder(d=5.6, h=4);
            translate([32-5, 52-5, top_height-2.5]) cylinder(d=5.6, h=4);

            translate([5, 5, -5]) cylinder(d=3.5, h=20);
            translate([5, 52-5, -5]) cylinder(d=3.5, h=20);
            translate([32-5, 5, -5]) cylinder(d=3.5, h=20);
            translate([32-5, 52-5, -5]) cylinder(d=3.5, h=20);
		}
		
	}	
}

module msx_case_cover() {
	
	difference() {
				
		color([0.5,0.5,0.5])
            translate([-2.5,-2.5,0])
                roundedcube(board_width+(wall*2)+5,board_length+(wall*2)+5,cover_height,5);

		// cutouts
			
        translate([0,0,0]) roundedcube(board_width+(wall*2)+0.1,board_length+(wall*2)+0.1,84-2,5);		
	
		translate([wall, wall, 0]) {

			// USB-C
			translate([32/2-(9.5/2),50,0]) cube([9.5,20,3.5+bottom_height]);            

			// boot button
			translate([32/2-(7.6/2),-5,0]) cube([7.6,20,7+bottom_height]);



		}
		
	}	
}

module msx_case_bottom() {
	
	difference() {
		color([0.5,0.5,0.5])
			roundedcube(board_width+(wall*2),board_length+(wall*2),bottom_height,5);
		
		// cutouts
		translate([3,10,1.5])
			roundedcube(board_width-3,board_length-17,10,5);
				
		translate([10.5,1.5,1.5])
			roundedcube(board_width-17.5,board_length+0.2,10,5);

		translate([wall, wall, 0]) {
			
            // board cutout
            translate([0,0,bottom_height-board_height])
                roundedcube(board_width+0.2,board_length+0.2,board_height+1,5);


		// bolt holes
		translate([5, 5, -11]) cylinder(d=3.2, h=25);
		translate([5, 52-5, -11]) cylinder(d=3.2, h=25);
		translate([32-5, 5, -11]) cylinder(d=3.2, h=25);
		translate([32-5, 52-5, -11]) cylinder(d=3.2, h=25);

		// nut holes
		translate([5, 5, -1]) cylinder(d=7, h=4, $fn=6);
		translate([5, 52-5, -1]) cylinder(d=7, h=4, $fn=6);
		translate([32-5, 5, -1]) cylinder(d=7, h=4, $fn=6);
		translate([32-5, 52-5, -1]) cylinder(d=7, h=4, $fn=6);

		}
		
	}	
}

// https://gist.github.com/tinkerology/ae257c5340a33ee2f149ff3ae97d9826
module roundedcube(xx, yy, height, radius)
{
    translate([0,0,height/2])
    hull()
    {
        translate([radius,radius,0])
        cylinder(height,radius,radius,true);

        translate([xx-radius,radius,0])
        cylinder(height,radius,radius,true);

        translate([xx-radius,yy-radius,0])
        cylinder(height,radius,radius,true);

        translate([radius,yy-radius,0])
        cylinder(height,radius,radius,true);
    }
}

  module rcube(xx, yy, height, radius)
{
    translate([-xx/2,-yy/2,0])
    hull()
    {
        translate([radius,radius,0])
        cylinder(height,radius,radius,true);

        translate([xx-radius,radius,0])
        cylinder(height,radius,radius,true);

        translate([xx-radius,yy-radius,0])
        cylinder(height,radius,radius,true);

        translate([radius,yy-radius,0])
        cylinder(height,radius,radius,true);
    }
}
