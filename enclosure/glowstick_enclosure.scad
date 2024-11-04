$fa = 1;
base_height = 75;
base_diameter = 110;
top_height = 20;
thick = 3;
led_diam = 40;
crystal_diam = 53;
wire_diam = 12;
gap = 0.2; // clearance between insert and bore.
inset = 3.0;

translate([base_diameter/2-15, base_diameter/2-15, 0]){
  difference(){
    // Hull
    translate([0, 0, base_height/2])
      cylinder(h=base_height, r=base_diameter/2, center=true);
    // bottom cut-out
    translate([0, 0, base_height/2+thick + 0.1])
      cylinder(h=base_height-thick, r=base_diameter/2-thick/2-inset, center=true);
    // Top cut-out
    translate([0, 0, base_height-top_height/2+1])
      cylinder(h=top_height+2, r=base_diameter/2-thick/2, center=true);

    // Wire hole
    translate([0, -base_diameter/2+thick*2, thick+wire_diam/2])
      rotate([90, 0, 0])
        scale([1, 0.75, 1])
          cylinder(h=thick*4, r=wire_diam/2);
  }
}

// Top
translate([-base_diameter/2+5, -base_diameter/2+5, 0]){
  union(){
    difference(){
      translate([0, 0, top_height/2])
        cylinder(h=top_height, r=base_diameter/2-thick/2-gap, center=true);
      translate([0, 0, top_height/2+thick/2+0.01])
        cylinder(h=top_height-thick, r=crystal_diam/2, center=true);
      translate([0, 0, thick/2-0.01])
        cylinder(h=thick+0.1, r=led_diam/2, center=true);
    }
  }
}