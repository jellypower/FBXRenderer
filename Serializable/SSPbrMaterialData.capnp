@0xa7237928ed8ce393;

using import "SSVector.capnp".Vector4fSrl;
using import "SSVector.capnp".Vector2fSrl;


struct SSPbrMaterialSrl {

    shaderName @0 :Text;

    txDiffuseName @1 :Text;
    txMetallicName @2 :Text;
    txNormalName @3 :Text;
    txEmissiveName @4 :Text;

    baseColorFactor @5 :Vector4fSrl;
    emissiveFactor @6 :Vector4fSrl;
    normalTextureScale @7 :Float32;
    metallicRoughnessFactor @8 :Vector2fSrl;

}