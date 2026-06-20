#!/usr/bin/env python3
"""Generate a PrusaSlicer-ready STL for the cylindrical pet button riser."""

import math
import struct
from pathlib import Path


BUTTON_DIAMETER = 88.0
INNER_CLEARANCE = 1.0
OUTER_WALL = 4.0
TOTAL_HEIGHT = 11.0
LEDGE_TOP_HEIGHT = 7.0
LEDGE_THICKNESS = 2.0
LEDGE_DEPTH = 5.0
SIDE_VENT_WIDTH = 28.0
FRONT_VENT_MULTIPLIER = 1.35
SIDE_VENT_BOTTOM = 1.0
SIDE_VENT_HEIGHT = 4.0
SEGMENTS = 192

INNER_RADIUS = (BUTTON_DIAMETER + INNER_CLEARANCE) / 2.0
OUTER_RADIUS = INNER_RADIUS + OUTER_WALL
LEDGE_INNER_RADIUS = INNER_RADIUS - LEDGE_DEPTH
LEDGE_BOTTOM_HEIGHT = LEDGE_TOP_HEIGHT - LEDGE_THICKNESS
VENT_TOP_Z = SIDE_VENT_BOTTOM + SIDE_VENT_HEIGHT


triangles = []


def normal(a, b, c):
    ux, uy, uz = (b[i] - a[i] for i in range(3))
    vx, vy, vz = (c[i] - a[i] for i in range(3))
    nx = uy * vz - uz * vy
    ny = uz * vx - ux * vz
    nz = ux * vy - uy * vx
    length = math.sqrt(nx * nx + ny * ny + nz * nz)
    if length == 0:
        return (0.0, 0.0, 0.0)
    return (nx / length, ny / length, nz / length)


def add_triangle(a, b, c):
    triangles.append((normal(a, b, c), a, b, c))


def add_quad(a, b, c, d):
    add_triangle(a, b, c)
    add_triangle(a, c, d)


def point(radius, theta, z):
    return (radius * math.cos(theta), radius * math.sin(theta), z)


def is_vent_gap(theta):
    theta = theta % (2.0 * math.pi)
    vents = [
        (0.0, SIDE_VENT_WIDTH * FRONT_VENT_MULTIPLIER),
        (math.pi / 2.0, SIDE_VENT_WIDTH),
        (math.pi, SIDE_VENT_WIDTH),
        (math.pi * 1.5, SIDE_VENT_WIDTH),
    ]
    for center, width in vents:
        half = (width / 2.0) / OUTER_RADIUS
        delta = abs(math.atan2(math.sin(theta - center), math.cos(theta - center)))
        if delta <= half:
            return True
    return False


def add_horizontal_ring(inner_r, outer_r, z, include_segment, upward=True):
    for i in range(SEGMENTS):
        t0 = 2.0 * math.pi * i / SEGMENTS
        t1 = 2.0 * math.pi * (i + 1) / SEGMENTS
        if not include_segment((t0 + t1) / 2.0):
            continue
        i0 = point(inner_r, t0, z)
        i1 = point(inner_r, t1, z)
        o0 = point(outer_r, t0, z)
        o1 = point(outer_r, t1, z)
        if upward:
            add_quad(i0, o0, o1, i1)
        else:
            add_quad(i1, o1, o0, i0)


def add_cylinder(radius, z0, z1, include_segment, outward=True):
    if z1 <= z0:
        return
    for i in range(SEGMENTS):
        t0 = 2.0 * math.pi * i / SEGMENTS
        t1 = 2.0 * math.pi * (i + 1) / SEGMENTS
        if not include_segment((t0 + t1) / 2.0):
            continue
        p00 = point(radius, t0, z0)
        p10 = point(radius, t1, z0)
        p11 = point(radius, t1, z1)
        p01 = point(radius, t0, z1)
        if outward:
            add_quad(p00, p10, p11, p01)
        else:
            add_quad(p01, p11, p10, p00)


def add_radial_cap(theta, inner_r, outer_r, z0, z1, reverse=False):
    i0 = point(inner_r, theta, z0)
    o0 = point(outer_r, theta, z0)
    o1 = point(outer_r, theta, z1)
    i1 = point(inner_r, theta, z1)
    if reverse:
        add_quad(i0, i1, o1, o0)
    else:
        add_quad(i0, o0, o1, i1)


def add_sector_caps(inner_r, outer_r, z0, z1):
    for i in range(SEGMENTS):
        mid = 2.0 * math.pi * (i + 0.5) / SEGMENTS
        if is_vent_gap(mid):
            continue
        t0 = 2.0 * math.pi * i / SEGMENTS
        t1 = 2.0 * math.pi * (i + 1) / SEGMENTS
        prev_mid = 2.0 * math.pi * (i - 0.5) / SEGMENTS
        next_mid = 2.0 * math.pi * (i + 1.5) / SEGMENTS
        if is_vent_gap(prev_mid):
            add_radial_cap(t0, inner_r, outer_r, z0, z1, reverse=True)
        if is_vent_gap(next_mid):
            add_radial_cap(t1, inner_r, outer_r, z0, z1, reverse=False)


def add_tri_prism(points_xy, z0, z1):
    bottom = [(x, y, z0) for x, y in points_xy]
    top = [(x, y, z1) for x, y in points_xy]
    add_triangle(bottom[2], bottom[1], bottom[0])
    add_triangle(top[0], top[1], top[2])
    for i in range(3):
        j = (i + 1) % 3
        add_quad(bottom[i], bottom[j], top[j], top[i])


full = lambda _theta: True
vent_only = lambda theta: is_vent_gap(theta)
solid_only = lambda theta: not is_vent_gap(theta)

# Simple hollow cylindrical wall with lower acoustic vents.
add_horizontal_ring(INNER_RADIUS, OUTER_RADIUS, 0.0, full, upward=False)
add_cylinder(OUTER_RADIUS, 0.0, SIDE_VENT_BOTTOM, full, outward=True)
add_cylinder(INNER_RADIUS, 0.0, SIDE_VENT_BOTTOM, full, outward=False)
add_cylinder(OUTER_RADIUS, SIDE_VENT_BOTTOM, VENT_TOP_Z, solid_only, outward=True)
add_cylinder(INNER_RADIUS, SIDE_VENT_BOTTOM, VENT_TOP_Z, solid_only, outward=False)
add_sector_caps(INNER_RADIUS, OUTER_RADIUS, SIDE_VENT_BOTTOM, VENT_TOP_Z)
add_horizontal_ring(INNER_RADIUS, OUTER_RADIUS, SIDE_VENT_BOTTOM, vent_only, upward=True)
add_horizontal_ring(INNER_RADIUS, OUTER_RADIUS, VENT_TOP_Z, vent_only, upward=False)

# Inner wall is interrupted where the shelf attaches, so no hidden full-height
# ledge is created.
add_cylinder(OUTER_RADIUS, VENT_TOP_Z, TOTAL_HEIGHT, full, outward=True)
add_cylinder(INNER_RADIUS, VENT_TOP_Z, LEDGE_BOTTOM_HEIGHT, full, outward=False)
add_cylinder(INNER_RADIUS, LEDGE_TOP_HEIGHT, TOTAL_HEIGHT, full, outward=False)
add_horizontal_ring(INNER_RADIUS, OUTER_RADIUS, TOTAL_HEIGHT, full, upward=True)

# Small suspended inner shelf/protrusion. It supports the button and leaves the
# lower chamber open.
add_horizontal_ring(LEDGE_INNER_RADIUS, INNER_RADIUS, LEDGE_BOTTOM_HEIGHT, full, upward=False)
add_horizontal_ring(LEDGE_INNER_RADIUS, INNER_RADIUS, LEDGE_TOP_HEIGHT, full, upward=True)
add_cylinder(LEDGE_INNER_RADIUS, LEDGE_BOTTOM_HEIGHT, LEDGE_TOP_HEIGHT, full, outward=False)

# Raised orientation arrow. Point this toward the microphone.
arrow = [
    (INNER_RADIUS + 1.0, -4.0),
    (OUTER_RADIUS - 1.0, 0.0),
    (INNER_RADIUS + 1.0, 4.0),
]
add_tri_prism(arrow, TOTAL_HEIGHT, TOTAL_HEIGHT + 0.8)


def write_binary_stl(path):
    header = b"Talking Pet Buttons cylindrical button-riser, units: millimeters"
    header = header[:80].ljust(80, b" ")
    with path.open("wb") as file:
        file.write(header)
        file.write(struct.pack("<I", len(triangles)))
        for n, a, b, c in triangles:
            file.write(struct.pack("<12fH", *(n + a + b + c), 0))


if __name__ == "__main__":
    output = Path(__file__).with_name("button-riser.stl")
    write_binary_stl(output)
    print(f"Wrote {output} with {len(triangles)} triangles")
