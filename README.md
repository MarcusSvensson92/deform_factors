# Deform Factors

An implementation of a technique presented in the paper ["Accurate and Efficient Lighting for Skinned Models"](http://vcg.isti.cnr.it/deformFactors/deformFactors.pdf). The technique improves accuracy of skinned surface normal transformations. It works by precomputing gradient values, so-called deform factors, between bone weights of neighboring vertices. The factors can then be used in run-time to calibrate surface normals.

![screenshot](https://user-images.githubusercontent.com/3328360/30772474-316044b2-a05c-11e7-8807-2dcf21ff2d49.png)

## Requirements

* Windows 10 x64
* Visual Studio 2015 x64 or higher
* Graphics Tools must be installed to run Debug

## Externals

* [DirectX 12](https://msdn.microsoft.com/en-us/library/windows/desktop/dn903821(v=vs.85).aspx)
* [Assimp](http://www.assimp.org/)
* [Unity Technologies](https://www.assetstore.unity3d.com/en/#!/content/39924) - The sample model is taken from the assets in "The Blacksmith"

## Notes

To test how well the technique performs in a memory bandwidth limited scenario, all vertex data except the reference normals are encoded. As the reference normals do not have the same precision as the encoded normals, minor visual differences between the implementations are to be expected.