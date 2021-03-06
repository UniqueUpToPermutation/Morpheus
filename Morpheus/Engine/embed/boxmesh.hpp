size_t mVertexCount = 24;
size_t mIndexCount = 36;

float mPositions[] = {
1, 1, 1, -1, 1, 1, -1, 1, -1, 1, 1, -1, 1, -1, -1, 1, 1, -1, -1, 1, -1, -1, -1, -1, -1, -1, -1, -1, 1, -1, -1, 1, 1, -1, -1, 1, -1, -1, 1, 1, -1, 1, 1, -1, -1, -1, -1, -1, 1, -1, 1, 1, 1, 1, 1, 1, -1, 1, -1, -1, -1, -1, 1, -1, 1, 1, 1, 1, 1, 1, -1, 1
};

float mUVs[] = {
0.625, 0.5, 0.875, 0.5, 0.875, 0.25, 0.625, 0.25, 0.375, 0.25, 0.625, 0.25, 0.625, 0, 0.375, 0, 0.375, 1, 0.625, 1, 0.625, 0.75, 0.375, 0.75, 0.125, 0.5, 0.375, 0.5, 0.375, 0.25, 0.125, 0.25, 0.375, 0.5, 0.625, 0.5, 0.625, 0.25, 0.375, 0.25, 0.375, 0.75, 0.625, 0.75, 0.625, 0.5, 0.375, 0.5
};

float mNormals[] = {
0, 1, -0, 0, 1, -0, 0, 1, -0, 0, 1, -0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -0, -1, 0, -0, -1, 0, -0, -1, 0, -0, 0, -1, -0, 0, -1, -0, 0, -1, -0, 0, -1, -0, 1, 0, -0, 1, 0, -0, 1, 0, -0, 1, 0, -0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1
};

float mTangents[] = {
-1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0
};

float mBitangents[] = {
0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0
};

uint32_t mIndices[] = {
3, 1, 0, 3, 2, 1, 7, 5, 4, 7, 6, 5, 11, 9, 8, 11, 10, 9, 15, 13, 12, 15, 14, 13, 19, 17, 16, 19, 18, 17, 23, 21, 20, 23, 22, 21
};
