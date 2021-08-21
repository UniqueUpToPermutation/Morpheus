#include <Engine/Frame.hpp>

#include <cereal/types/string.hpp>
#include <cereal/archives/portable_binary.hpp>

namespace Morpheus {
	std::atomic<FrameId> mFrameIdCounter;

	Frame::Frame(const std::filesystem::path& path) {
		mPath = path;
		mDevice = Device::Disk();
	}

	entt::entity Frame::SpawnDefaultCamera(Camera** cameraOut) {
		auto cameraNode = CreateEntity();
		auto& camera = Emplace<Camera>(cameraNode);

		if (cameraOut)
			*cameraOut = &camera;

		return cameraNode;
	}

	entt::entity Frame::CreateEntity(entt::entity parent) {
		auto e = mRegistry.create();
		mRegistry.emplace<HierarchyData>(e);
		AddChild(parent, e);
		return e;
	}

	void Frame::Destroy(entt::entity ent) {
		Orphan(ent);

		for (entt::entity child = GetFirstChild(ent); 
			child != entt::null; 
			child = GetNext(child)) 
			Destroy(child);

		mRegistry.destroy(ent);
	}

	Frame::Frame() {
		SetFrameId(mFrameIdCounter.fetch_add(1));
		mRoot = mRegistry.create();
		mRegistry.emplace<HierarchyData>(mRoot);
	}

	entt::meta_type Frame::GetType() const {
		return entt::resolve<Frame>();
	}

	entt::meta_any Frame::GetSourceMeta() const {
		return mPath;
	}

	void Frame::BinarySerialize(std::ostream& output) const {
		throw std::runtime_error("Cannot serialize! Use FrameIO instead!");
	}

	void Frame::BinaryDeserialize(std::istream& input) {
		throw std::runtime_error("Cannot deserialize! Use FrameIO instead!");
	}

	void Frame::BinarySerializeSource(
		const std::filesystem::path& workingPath, 
		cereal::PortableBinaryOutputArchive& output) const {
	
		auto relativePath = std::filesystem::relative(mPath, workingPath);
		output(relativePath.string());
	}

	void Frame::BinaryDeserializeSource(
		const std::filesystem::path& workingPath,
		cereal::PortableBinaryInputArchive& input) {

		std::string src;
		input(src);
		auto relativeSrc = std::filesystem::path(src);
		mPath = workingPath / relativeSrc;
	}

	BarrierOut Frame::MoveAsync(Device device, Context context) {
		Barrier barrier;

		for (auto& resource : mInternalResources) {
			barrier.Node().After(resource->MoveAsync(device, context));
		}

		return barrier;
	}
}