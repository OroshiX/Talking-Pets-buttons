// Talking Pet Buttons - parametric sound riser
// Open in OpenSCAD, adjust button_diameter and export STL.

$fn = 96;

button_diameter = 88;      // mm, measure the bottom of the recordable button
inner_clearance = 1.0;     // mm
outer_wall = 4.0;          // mm
riser_height = 8.0;        // mm
floor_thickness = 2.0;     // mm
lip_height = 3.0;          // mm
lip_thickness = 2.2;       // mm
speaker_hole_diameter = 38;
side_vent_width = 28;
side_vent_height = 4.0;
velcro_recess_depth = 0.8;
velcro_recess_width = 18;
velcro_recess_length = 54;

inner_diameter = button_diameter + inner_clearance;
outer_diameter = inner_diameter + outer_wall * 2;
total_height = riser_height + lip_height;

module rounded_cylinder(d, h) {
  cylinder(d = d, h = h);
}

module side_vent(angle, width, height) {
  rotate([0, 0, angle])
    translate([outer_diameter / 2 - outer_wall / 2, 0, floor_thickness + height / 2])
      cube([outer_wall + 3, width, height], center = true);
}

module velcro_recess(angle) {
  rotate([0, 0, angle])
    translate([0, outer_diameter * 0.22, -0.01])
      cube([velcro_recess_width, velcro_recess_length, velcro_recess_depth + 0.02], center = true);
}

difference() {
  union() {
    // Base disk that lifts the button away from the yoga tile.
    rounded_cylinder(outer_diameter, riser_height);

    // Low retaining lip. It keeps the button centered without trapping sound.
    difference() {
      translate([0, 0, riser_height])
        rounded_cylinder(outer_diameter, lip_height);
      translate([0, 0, riser_height - 0.1])
        rounded_cylinder(inner_diameter, lip_height + 0.3);
    }
  }

  // Main cavity for the button bottom.
  translate([0, 0, floor_thickness])
    rounded_cylinder(inner_diameter, total_height + 0.2);

  // Speaker relief hole under the button.
  translate([0, 0, -0.1])
    rounded_cylinder(speaker_hole_diameter, floor_thickness + 0.3);

  // One larger front vent should face the microphone.
  side_vent(0, side_vent_width * 1.35, side_vent_height);

  // Three secondary vents.
  side_vent(90, side_vent_width, side_vent_height);
  side_vent(180, side_vent_width, side_vent_height);
  side_vent(270, side_vent_width, side_vent_height);

  // Optional shallow recesses for Velcro or rubber pads underneath.
  velcro_recess(0);
  velcro_recess(180);
}

// Orientation marker: this notch points toward the microphone.
translate([outer_diameter / 2 - 5, 0, total_height + 0.4])
  linear_extrude(0.8)
    polygon(points = [[-4, -4], [5, 0], [-4, 4]]);
