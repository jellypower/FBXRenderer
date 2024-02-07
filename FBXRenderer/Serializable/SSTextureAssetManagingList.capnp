@0xe3482112333ca875;

struct SSTextureAssetManagingList{
  textureList @0 :List(TextureAssetPair);

  struct TextureAssetPair{
    textureName @0 :Text;
    texturePath @1 :Text;
  }
}
