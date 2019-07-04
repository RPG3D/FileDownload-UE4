# FileDownload-UE4
A http based file download plugin for Unreal Engine 4.17

## note
  The plugin support async IO, but disabled by default, you can enable it by changing the condition from 0 to true(DownloadTask.cpp DownloadTask::OnGetChunkCompleted)
