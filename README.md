# FileDownload-UE4

A http based file download plugin for Unreal Engine 5+

## feature

1.breakpoint resume.(save task progress to json on stop, read from json on resume)

2.block based download.(use http RANGE feature download large file, up to 4GB, you can change int32 to int64 for 4GB+ file)

3.async IO write, no IO block on game thread

## usages
Pseudo code
```
--add task on button clicked event 
local DownloadMgr = UE4.UGameplayStatics.SpawnObject(UE4.UFileDownloadManager)

local url = 'https://www.blender.org/download/release/Blender3.2/blender-3.2.0-windows-x64.zip'

local TaskID = DownloadMgr:AddTaskByUrl(url, '', '')

--get progress umg tick
local TaskInfo = DownloadMgr:GetTaskInfo(TaskID)
local percent = TaskInfo.CurrentSize / TaskInfo.TotalSize
--update widget

```
