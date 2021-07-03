#include <Engine/Resources/RawGeometry.hpp>

#include <iostream>
#include <fstream>

using namespace std;
using namespace Morpheus;

int main(int argc, const char *argv[]) {

	if (argc != 3)
		std::cout << "Incorrect Number of Arguments!" << std::endl;

	DG::LayoutElement elePosition{0, 0, 3, DG::VT_FLOAT32};
	DG::LayoutElement eleUV{1, 1, 2, DG::VT_FLOAT32};
	DG::LayoutElement eleNormal{2, 2, 3, DG::VT_FLOAT32};
	DG::LayoutElement eleTangent{3, 3, 3, DG::VT_FLOAT32};
	DG::LayoutElement eleBitangent{4, 4, 3, DG::VT_FLOAT32};

	std::vector<DG::LayoutElement> layoutElements = {
		elePosition,
		eleUV,
		eleNormal,
		eleTangent,
		eleBitangent
	};

	VertexLayout vLayout;
	vLayout.mElements = layoutElements;
	vLayout.mPosition = 0;
	vLayout.mUV = 1;
	vLayout.mNormal = 2;
	vLayout.mTangent = 3;
	vLayout.mBitangent = 4;

	LoadParams<Geometry> params;
	params.mSource = argv[1];
	params.mVertexLayout = vLayout;

	RawGeometry rawGeo;
	rawGeo.LoadAssimp(params);

	std::vector<float> positions;
	std::vector<float> uvs;
	std::vector<float> normals;
	std::vector<float> tangents;
	std::vector<float> bitangents;
	std::vector<uint32_t> indices;

	auto& rawPositions = rawGeo.GetVertexData(0);
	auto& rawUVs = rawGeo.GetVertexData(1);
	auto& rawNormals = rawGeo.GetVertexData(2);
	auto& rawTangents = rawGeo.GetVertexData(3);
	auto& rawBitangents = rawGeo.GetVertexData(4);
	auto& rawIndex = rawGeo.GetIndexData();

	positions.resize(rawPositions.size() / sizeof(float));
	uvs.resize(rawUVs.size() / sizeof(float));
	normals.resize(rawNormals.size() / sizeof(float));
	tangents.resize(rawTangents.size() / sizeof(float));
	bitangents.resize(rawBitangents.size() / sizeof(float));
	indices.resize(rawIndex.size() / sizeof(uint32_t));

	std::memcpy(&positions[0], &rawPositions[0], rawPositions.size());
	std::memcpy(&uvs[0], &rawUVs[0], rawUVs.size());
	std::memcpy(&normals[0], &rawNormals[0], rawNormals.size());
	std::memcpy(&tangents[0], &rawTangents[0], rawTangents.size());
	std::memcpy(&bitangents[0], &rawBitangents[0], rawBitangents.size());
	std::memcpy(&indices[0], &rawIndex[0], rawIndex.size());

	ofstream out(argv[2]);

	out << "size_t mVertexCount = " << positions.size() / 3 << ";" << std::endl;
	out << "size_t mIndexCount = " << indices.size() << ";" << std::endl;
	out << std::endl;

	auto print_buffer = [&](const std::string& name, const std::vector<float>& data) {
		out << "float " << name << "[] = {" << std::endl;
		if (data.size() > 0) {
			out << data[0];
			for (int i = 1; i < data.size(); ++i) {
				out << ", " << data[i];
			}
		}

		out << std::endl << "};" << std::endl << std::endl;
	};

	print_buffer("mPositions", positions);
	print_buffer("mUVs", uvs);
	print_buffer("mNormals", normals);
	print_buffer("mTangents", tangents);
	print_buffer("mBitangents", bitangents);

	out << "uint32_t mIndices[] = {" << std::endl;
	if (indices.size() > 0) {
		out << indices[0];
		for (int i = 1; i < indices.size(); ++i) {
			out << ", " << indices[i];
		}
	}

	out << std::endl << "};";
	out << std::endl;
}