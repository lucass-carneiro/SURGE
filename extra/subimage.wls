#!/usr/bin/env wolframscript
(* ::Package:: *)

(* Some naming conventions *)
(* ow -> Original image width in pixels *)
(* oh -> Original image height in pixels *)
(* sx -> X origin of the subimage in pixels *)
(* sy -> Y origin of the subimage in pixels *)
(* sw -> Width of the subimage in pixels *)
(* sh -> Height of the subimage in pixels *)


(* Map from pixel coordinates to uv coordinates *)


x2u[x_] = InterpolatingPolynomial[{{0, 0}, {ow, 1}}, x];
y2v[y_] = InterpolatingPolynomial[{{0, 0}, {oh, 1}}, y];


(* Finding the transformation matrix *)


subimage = {{a, b, c}, {d, e, f}, {g, h, i}};

eqs = {
  subimage . {{0}, {0}, {1}} == {{x2u[sx]}, {y2v[sy]}, {1}},
  subimage . {{1}, {0}, {1}} == {{x2u[sx + sw]}, {y2v[sy]}, {1}},
  subimage . {{0}, {1}, {1}} == {{x2u[sx]}, {y2v[sy + sh]}, {1}},
  subimage . {{1}, {1}, {1}} == {{x2u[sx + sw]}, {y2v[sy + sh]}, {1}}
};

sol = Flatten[Solve[eqs, {a, b, c, d, e, f, g, h, i}]];

MatrixForm[subimage//.sol]
