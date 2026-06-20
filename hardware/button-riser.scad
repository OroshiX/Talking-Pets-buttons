// Talking Pet Buttons - parametric cylindrical sound riser
// Open in OpenSCAD, adjust button_diameter and export STL.

$fn = 96;

button_diameter = 88;       // mm, measure the bottom of the recordable button
inner_clearance = 1.0;      // mm
outer_wall = 4.0;           // mm, simple cylindrical wall thickness
total_height = 11.0;        // mm, full height of the support cylinder
ledge_top_height = 7.0;     // mm, top of the small inner shelf where the button rests
ledge_thickness = 2.0;      // mm, vertical thickness of the shelf
ledge_depth = 5.0;          // mm, how far the shelf protrudes inward
side_vent_width = 28.0;     // mm
side_vent_height = 4.0;     // mm
side_vent_bottom = 1.0;     // mm, keep vents below the inner shelf

inner_diameter = button_diameter + inner_clearance;
outer_diameter = inner_diameter + outer_wall * 2;
inner_radius = inner_diameter / 2;
outer_radius = outer_diameter / 2;
ledge_inner_radius = inner_radius - ledge_depth;
ledge_bottom_height = ledge_top_height - ledge_thickness;

module cylinder_wall() {
  difference() {
    cylinder(d = outer_diameter, h = total_height);
    translate([0, 0, -0.1])
      cylinder(d = inner_diameter, h = total_height + 0.2);
  }
}

module inner_ledge() {
  translate([0, 0, ledge_bottom_height])
    difference() {
      cylinder(r = inner_radius, h = ledge_thickness);
      translate([0, 0, -0.1])
        cylinder(r = ledge_inner_radius, h = ledge_thickness + 0.2);
    }
}

module radial_side_vent(angle, width, height) {
  rotate([0, 0, angle])
    translate([(inner_radius + outer_radius) / 2, 0, side_vent_bottom + height / 2])
      cube([outer_radius - inner_radius + 4, width, height], center = true);
}

difference() {
  union() {
    // Main body: a simple hollow cylinder.
    cylinder_wall();

    // Small suspended inner shelf. It supports the button but does not continue
    // down to the bottom, leaving an air chamber below it.
    inner_ledge();
  }

  // Radial vents through the cylindrical wall, below the inner shelf.
  radial_side_vent(0, side_vent_width * 1.35, side_vent_height);
  radial_side_vent(90, side_vent_width, side_vent_height);
  radial_side_vent(180, side_vent_width, side_vent_height);
  radial_side_vent(270, side_vent_width, side_vent_height);
}

// Orientation marker: this arrow points toward the microphone.
translate([outer_radius - 5, 0, total_height + 0.4])
  linear_extrude(0.8)
    polygon(points = [[-4, -4], [5, 0], [-4, 4]]);
