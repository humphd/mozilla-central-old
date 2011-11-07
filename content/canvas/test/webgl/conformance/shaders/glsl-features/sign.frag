// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

precision mediump float;

varying vec2 vTexcoord;
varying vec4 vColor;

void main()
{
   gl_FragColor = vec4(
     sign(vTexcoord.x * 2.0 - 1.0),
     sign(vTexcoord.y * 2.0 - 1.0),
     0,
     1);
}


