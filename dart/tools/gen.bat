dart pub get
protoc --dart_out=grpc:lib/src/generated -I../proto ../proto/rcr.proto