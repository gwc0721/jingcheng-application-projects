Release Note:

1.0.0.1

rev. 1.0.0.2
#20150115: 
	问题：当User Mode Driver提供的缓存小于应用程序请求的读写大小是。Kernel Driver没有检查直接copy，导致blue screen panic。
	解决：(1) 对于纯Read或者Write请求(例如Read(),或者Write())的大小超过User Mode Driver的缓存大小时，将应用程序的一个请求分解成多个请求发送给User Mode Driver。这种方法是模拟实际Device的传输限制。（LBA 24模式的128KB限制）
	(2) 对于读写请求(例如SCSI_PASSTHROW)，如果User Mode的缓存不够，则对于RequestExchange请求返回错误，试图终止本次Device的服务。同时完成应用程序的请求并且会错误。
	残留问题：当对于读写请求的缓存不够时，RequestExchange请求返回错误。User Mode Driver可以检测错误并且终止RequestExchange线程。如果遇到应用程序重试，则User Mode无法响应(没有RequestEchange线程),导致应用程序等待。
	User Mode Driver必须在发现错误后发出Disconnect和Unmount请求，已终止Device。但是目前RequestExchange线程的终止无法通知主线程。
	影响：目前User Mode Driver的缓存设置为32MB，应该可以满足LBA 48的请求。一般SCSI_PASSTHROW不需要如此大的空间。即使问题发生，只需手动终止User Mode Driver即可。
	思考(1):RequestExchange线程错误终止时通知主线程
	    (2)：RequestExchange线程即使遇到错误，也不退出，重新发出RequestExchange请求，等待处理下一个请求。期望应用程序重试失败后自动终止。
		(3):有Kernel Driver处理错误，并且自动清空请求队列然后dismount。
	