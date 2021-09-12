#include <Engine/Resources/FrameIO.hpp>
#include <Engine/Resources/Cache.hpp>
#include <Engine/Systems/System.hpp>
#include <Engine/Resources/Material.hpp>
#include <Engine/Resources/Texture.hpp>

#include <cereal/types/unordered_map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/portable_binary.hpp>

#include <fstream>

using namespace entt;

namespace Morpheus {

	std::unique_ptr<cereal::PortableBinaryInputArchive> 
		MakePortableBinaryInputArchive(std::istream& stream) {
		return std::make_unique<cereal::PortableBinaryInputArchive>(stream);
	}

	std::unique_ptr<cereal::PortableBinaryOutputArchive>
		MakePortableBinaryOutputArchive(std::ostream& stream) {
		return std::make_unique<cereal::PortableBinaryOutputArchive>(stream);
	}

	void WriteInt64(cereal::PortableBinaryOutputArchive& arr, size_t sz) {
		arr(sz);
	}

	void WriteEntity(cereal::PortableBinaryOutputArchive& arr, entt::entity e) {
		arr(e);
	}

	void ReadInt64(cereal::PortableBinaryInputArchive& arr, size_t& sz) {
		arr(sz);
	}

	void ReadEntity(cereal::PortableBinaryInputArchive& arr, entt::entity& e) {
		arr(e);
	}

	template <typename Archive>
	void FrameDependency::serialize(Archive& ar) {
		ar(mType);
		ar(mIdentifier);
		ar(mTypeName);
		ar(mBlob);
	}

	template <class Archive>
	void serialize(Archive& archive, ArchiveBlobPointer& m) {
		archive(CEREAL_NVP(m.mBegin));
		archive(CEREAL_NVP(m.mSize));
	}

	void FrameTable::Write(std::ostream& stream) {
		cereal::PortableBinaryOutputArchive out(stream);
	
		out(mRoot);
		out(mCamera);
		out(mInternalResourceTable);
		out(mTypeDirectory);
		out(mNameToEntity);
		out(mDependencies);
		out(mCurrentId);
	}

	void FrameTable::Read(std::istream& stream) {
		cereal::PortableBinaryInputArchive in(stream);

		in(mRoot);
		in(mCamera);
		in(mInternalResourceTable);
		in(mTypeDirectory);
		in(mNameToEntity);
		in(mDependencies);
		in(mCurrentId);
	}

	template <>
	SerializableType MakeSerializableComponentType<Material>() {
		return SerializableType(new MaterialSerializableType());
	}

	void FrameTable::FindAndThenRead(std::istream& stream) {
		ArchiveBlobPointer tableBlob;

		{
			cereal::PortableBinaryInputArchive archive(stream);
			archive(tableBlob);
		}

		// Read the frame table
		stream.seekg(tableBlob.mBegin);
		Read(stream);
	}

	void BuildSerializationSetRecursive(Frame& frame, entt::entity e, SerializationSet& out) {
		for (auto child = frame.GetFirstChild(e); 
			child != entt::null; 
			child = frame.GetNext(child)) {

			if (frame.Has<SubFrameComponent>(e)) {
				out.mSubFrames.emplace_back(child);
			} else {
				out.mToSerialize.emplace_back(child);
				BuildSerializationSetRecursive(frame, child, out);
			}
		}
	}

	SerializationSet BuildSerializationSet(Frame& frame) {
		SerializationSet set;
		BuildSerializationSetRecursive(frame, frame.GetRoot(), set);
		set.mToSerialize.emplace_back(frame.GetRoot());
		return set;
	}

	bool FrameTable::FindComponent(const std::string& component, 
		ArchiveBlobPointer* out) {
		
		auto it = mTypeDirectory.find(component);
	
		if (it != mTypeDirectory.end()) {
			*out = it->second;
			return true;	
		}

		return false;
	}

	bool FrameTable::FindComponent(const SerializableType& type, 
		ArchiveBlobPointer* out) {
		std::string str(type->GetType().info().name());
		return FindComponent(str, out);
	}

	void FrameTable::WriteResourceData(std::ostream& stream) {
		for (auto& dependency : mDependencies) {
			auto resourceStart = stream.tellp();
			if (dependency.second.mType == DependencyEntryType::INTERNAL) {
				dependency.second.mResource->BinarySerialize(stream, this);
			}
			auto resourceEnd = stream.tellp();

			dependency.second.mBlob.mBegin = resourceStart;
			dependency.second.mBlob.mSize = resourceEnd - resourceStart;
		}
	}

	ResourceId FrameTable::AddDependency(Handle<IResource> resource) {
		auto it = mPointerToId.find(resource);

		if (it != mPointerToId.end()) {
			return it->second;
		}

		std::string typeName(resource->GetType().info().name());

		FrameDependency dependency;
		dependency.mIdentifier.mEntity = resource->GetEntity();
		dependency.mResource = resource;
		dependency.mTypeName = typeName;

		auto parentFrame = resource->GetFrame();

		if (parentFrame == mFrame) {
			dependency.mIdentifier.mEntity = resource->GetEntity();
		} else {
			dependency.mIdentifier = resource->GetUniversalId();
		}
		
		if (parentFrame) {
			if (parentFrame == mFrame) {
				dependency.mType = DependencyEntryType::INTERNAL;
			} else {
				dependency.mType = DependencyEntryType::EXTERNAL_FRAME_RESOURCE;
			}
		} else {
			if (resource->GetType() == entt::resolve<Frame>()) {
				dependency.mType == DependencyEntryType::EXTERNAL_FRAME;
			} else {
				dependency.mType = DependencyEntryType::EXTERNAL;
			}
		}

		auto id = mCurrentId++;
		mDependencies[id] = dependency;
		mPointerToId[resource] = id;
		return id;
	}

	Handle<IResource> FrameTable::GetDependency(ResourceId id) const {
		auto it = mDependencies.find(id);

		if (it != mDependencies.end()) {
			return it->second.mResource;
		} else {
			throw std::runtime_error("Could not find dependency!");
		}
	}

	void FrameTable::ReadFramesRecursive(
		ResourceCache& cache,
		const std::vector<SerializableType>& types) {

		auto subframeView = mFrame->Registry().view<SubFrameComponent>();

		for (auto e : subframeView) {
			auto& subframe = subframeView.get<SubFrameComponent>(e);

			// Load all subframes
			if (subframe.mFrame->GetDevice().IsDisk()) {
				*subframe.mFrame = std::move(FrameIO::Load(
					subframe.mFrame->GetPath(), cache, types));
			}
		}
	}

	void FrameTable::ReadComponents(std::istream& stream,
		const std::filesystem::path& workingPath,
		const std::vector<SerializableType>& types) {

		for (auto type : types) {
			if (type->IsComponent()) {
				std::string type_name(type->GetType().info().name());
				auto blob = mTypeDirectory.find(type_name);

				if (blob != mTypeDirectory.end()) {
					stream.seekg(blob->second.mBegin);
					type->Deserialize(nullptr, workingPath, 
						mFrame->Registry(), stream, this, nullptr);
				}
			}
		}
	}

	void FrameTable::ReadResourceComponents(ResourceCache& cache,
		std::istream& stream,
		const std::filesystem::path& workingPath,
		const std::vector<SerializableType>& types) {

		std::unordered_map<entt::entity, Handle<IResource>> resources;

		std::vector<SerializableType> resourceTypes;

		for (auto type : types) {
			if (type->IsResource()) {
				resourceTypes.emplace_back(type);
			}
		}

		// Make sure we save resources in order of priority
		std::sort(resourceTypes.begin(), resourceTypes.end(),
			[](const SerializableType& t1, const SerializableType& t2) {
			return t1->GetLoadPriority() > t2->GetLoadPriority();
		});

		for (auto type : resourceTypes) {
			std::string type_name(type->GetType().info().name());
			auto blob = mTypeDirectory.find(type_name);

			if (blob != mTypeDirectory.end()) {
				stream.seekg(blob->second.mBegin);

				// Deserialize internal resource components
				type->Deserialize(&cache, workingPath, mFrame->Registry(), stream, this, &resources);
				
				// Deserialize external resource components
				type->Deserialize(&cache, workingPath, mExternalResourceRegistry, stream, this, &resources);
			}
		}

		// Link up internal + external dependencies
		for (auto it : mDependencies) {
			auto& dep = it.second;

			if (dep.mType == DependencyEntryType::INTERNAL 
				|| dep.mType == DependencyEntryType::EXTERNAL) {
				auto resIt = resources.find(dep.mIdentifier.mEntity);

				if (resIt == resources.end()) {
					throw std::runtime_error("Could not find resource!");
				}

				dep.mResource = resIt->second;
			}
		}
	}

	void FrameTable::ReadResourceData(std::istream& stream) {
		for (auto& dependency : mDependencies) {
			if (dependency.second.mType == DependencyEntryType::INTERNAL) {
				stream.seekg(dependency.second.mBlob.mBegin);
				dependency.second.mResource->BinaryDeserialize(stream, this);
			}
		}
	}

	void FrameTable::WriteComponents(
		const std::filesystem::path& workingPath,
		std::ostream& stream, 
		const std::vector<SerializableType>& types) {
		// Write components, compute resource tables
		for (auto type : types) {
			if (type->IsComponent()) {
				// Serialize
				ArchiveBlobPointer blob;
				blob.mBegin = stream.tellp();
				type->Serialize(workingPath, mFrame->Registry(), stream, this);
				blob.mSize = stream.tellp() - blob.mBegin;

				// Write entry into frame table
				std::string type_name(type->GetType().info().name());
				mTypeDirectory[type_name] = blob;
			}
		}
	}

	void FrameTable::WriteResourceComponents(
		const std::filesystem::path& workingPath,
		std::ostream& stream, 
		const std::vector<SerializableType>& types) {

		std::vector<SerializableType> resourceTypes;

		for (auto type : types) {
			if (type->IsResource()) {
				resourceTypes.emplace_back(type);
			}
		}

		// Make sure we save resources in order of priority
		std::sort(resourceTypes.begin(), resourceTypes.end(),
			[](const SerializableType& t1, const SerializableType& t2) {
			return t1->GetLoadPriority() < t2->GetLoadPriority();
		});

		// Write components, compute resource tables
		for (auto type : resourceTypes) {
			if (type->IsResource()) {
				// Serialize
				ArchiveBlobPointer blob;
				blob.mBegin = stream.tellp();

				// Serialize internal resources
				type->Serialize(workingPath, mFrame->Registry(), stream, this);
				// Serialize external resources
				type->Serialize(workingPath, mExternalResourceRegistry, stream, this);

				blob.mSize = stream.tellp() - blob.mBegin;

				// Write entry into frame table
				std::string type_name(type->GetType().info().name());
				mTypeDirectory[type_name] = blob;
			}
		}
	}

	void FrameTable::FlushToFrame() {
		mFrame->mInternalResourceTable = std::move(mInternalResourceTable);
		mFrame->mNameToEntity = std::move(mNameToEntity);

		for (auto& dependency : mDependencies) {
			if (dependency.second.mType == DependencyEntryType::INTERNAL) {
				mFrame->mEntityToResource[dependency.second.mIdentifier.mEntity] 
					= dependency.second.mResource;
			}
		}

		mFrame->mRoot = mRoot;
		mFrame->mCamera = mCamera;
	}

	void FrameTable::InitFromFrame() {
		mInternalResourceTable = mFrame->mInternalResourceTable;
		mNameToEntity = mNameToEntity;

		mRoot = mFrame->mRoot;
		mCamera = mFrame->mCamera;
	}

	void FrameIO::Save(Frame& frame,
			std::ostream& stream, 
			const std::vector<SerializableType>& types) {

		SerializationSet set = BuildSerializationSet(frame);

		ArchiveBlobPointer tableBlob;

		// Build the resource table
		// First, add subframes to resource table
		FrameTable table(&frame);
		table.InitFromFrame();

		for (auto& subFrame : set.mSubFrames) {
			auto& subFrameComponent = frame.Registry().get<SubFrameComponent>(subFrame);
			table.AddDependency(subFrameComponent.mFrame.DownCast<IResource>());
		}

		auto streamBeginPos = stream.tellp();
		{
			cereal::PortableBinaryOutputArchive archive(stream);
			archive(tableBlob);
		}

		// Write components and resources
		table.WriteComponents(frame.GetPath(), stream, types);
		table.WriteResourceComponents(frame.GetPath(), stream, types);		

		// Write internal resource data
		table.WriteResourceData(stream);

		// Write the table itself
		tableBlob.mBegin = stream.tellp();
		table.Write(stream);
		tableBlob.mSize = stream.tellp() - tableBlob.mBegin;

		// Overwrite the table blob
		{
			stream.seekp(streamBeginPos);
			cereal::PortableBinaryOutputArchive archive(stream);
			archive(tableBlob);
		}
	}
	
	void FrameIO::Save(Frame& frame,
		const std::filesystem::path& path,
		const std::vector<SerializableType>& componentTypes) {

		std::ofstream f(path, std::ios::binary);

		if (!f.good()) {
			throw std::runtime_error("Failed to open file!");
		}

		Save(frame, f, componentTypes);
	}

	Frame FrameIO::Load(
		std::istream& stream,
		const std::filesystem::path& workingPath,
		ResourceCache& cache,
		const std::vector<SerializableType>& componentTypes) {

		ArchiveBlobPointer tableBlob;
		Frame frame;

		// Build the resource table
		FrameTable table(&frame);

		table.FindAndThenRead(stream);
		table.ReadResourceComponents(cache, stream, workingPath, componentTypes);
		table.ReadComponents(stream, workingPath, componentTypes);
		table.ReadFramesRecursive(cache, componentTypes);
		table.FlushToFrame();

		return frame;
	}

	Frame FrameIO::Load(const std::filesystem::path& path,
		ResourceCache& cache,
		const std::vector<SerializableType>& componentTypes) {
		std::ifstream f(path, std::ios::binary);

		if (!f.good()) {
			throw std::runtime_error("Failed to open file!");
		}

		return Load(f, path.parent_path(), cache, componentTypes);
	}
}