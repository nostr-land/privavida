//
//  Networking.swift
//  privavida-ios
//
//  Created by Bartholomew Joyce on 20/05/2023.
//

import Foundation
import Starscream

@objc public class Networking: NSObject {
    @objc static let sharedInstance = Networking()
    
    @objc func websocketOpen(url: UnsafePointer<Int8>?, userData: UnsafeMutableRawPointer?) -> UnsafeMutableRawPointer? {
        guard let url = url else {
            return nil
        }

        let url2 = URL(string: String(cString: url))
        guard let url2 = url2 else {
            return nil
        }

        print("WebSocket open: \(String(cString: url))")
        let wsHandle = WebSocketHandle(url: url2, userData: userData)
        return Unmanaged.passRetained(wsHandle).toOpaque()
    }

    @objc func websocketSend(ws: UnsafeMutableRawPointer?, data: UnsafePointer<Int8>?) {
        guard let ws = ws, let data = data else {
            return
        }

        let unmanaged = Unmanaged<WebSocketHandle>.fromOpaque(ws)
        let wsHandle = unmanaged.takeUnretainedValue()

        let message = String(cString: data)
        wsHandle.socket.write(string: message)
    }

    @objc func websocketClose(ws: UnsafeMutableRawPointer?, code: UInt16, reason: UnsafePointer<Int8>?) {
        guard let ws = ws, let reason = reason else {
            return
        }

        let unmanaged = Unmanaged<WebSocketHandle>.fromOpaque(ws)
        let wsHandle = unmanaged.takeUnretainedValue()

        let message = String(cString: reason)
        print("WebSocket close: code=\(code), reason=\(message)")
    }

    @objc func httpRequestSend(url: UnsafePointer<Int8>?, userData: UnsafeMutableRawPointer?) {
        guard let url = url else {
            return
        }

        let url2 = URL(string: String(cString: url))
        guard let url2 = url2 else {
            return
        }

        let request = URLRequest(url: url2)

        let task = URLSession.shared.dataTask(with: request) { (data, response, error) in
            var event = AppHttpEvent()
            event.data_length = 0;
            event.data = nil;
            event.user_data = userData;

            if error != nil {
                event.type = HTTP_RESPONSE_ERROR;
                event.status_code = -1;
                app_http_event(&event)
                return
            }

            guard let httpResponse = response as? HTTPURLResponse else {
                event.type = HTTP_RESPONSE_ERROR;
                event.status_code = -1;
                app_http_event(&event)
                return
            }

            event.status_code = Int32(httpResponse.statusCode)

            if let responseData = data {
                responseData.withUnsafeBytes { ptr in
                    var dataEvent = event
                    dataEvent.type = HTTP_RESPONSE_SUCCESS
                    dataEvent.data_length = Int32(responseData.count)
                    dataEvent.data = unsafeBitCast(ptr.baseAddress, to: UnsafePointer<UInt8>.self)
                    app_http_event(&dataEvent)
                }
            }
        }

        task.resume()
    }

}

class WebSocketHandle: WebSocketDelegate {
    lazy var socket = {
        let req = URLRequest(url: url)
        let socket = WebSocket(request: req, compressionHandler: .none)
        socket.delegate = self
        return socket
    }()
    let userData: UnsafeMutableRawPointer?
    let url: URL

    init(url: URL, userData: UnsafeMutableRawPointer?) {
        self.url = url
        self.userData = userData
        self.socket.connect()
    }

    public func didReceive(event: WebSocketEvent, client: WebSocket) {
        switch event {
        case .connected:
            print("WebSocket connected")
            var event = AppWebsocketEvent()
            event.type = WEBSOCKET_OPEN
            event.socket = Unmanaged.passUnretained(self).toOpaque()
            event.user_data = userData
            app_websocket_event(&event)

        case .disconnected:
            print("WebSocket disconnected")
            var event = AppWebsocketEvent()
            event.type = WEBSOCKET_CLOSE
            event.socket = Unmanaged.passUnretained(self).toOpaque()
            event.user_data = userData
            app_websocket_event(&event)

        case .cancelled, .error:
            print("WebSocket error")
            var event = AppWebsocketEvent()
            event.type = WEBSOCKET_ERROR
            event.socket = Unmanaged.passUnretained(self).toOpaque()
            event.user_data = userData
            app_websocket_event(&event)

        case .text(let txt):
            let cString = txt.utf8CString
            cString.withUnsafeBytes { ptr in
                var event = AppWebsocketEvent()
                event.type = WEBSOCKET_MESSAGE
                event.socket = Unmanaged.passUnretained(self).toOpaque()
                event.user_data = userData
                event.data = unsafeBitCast(ptr.baseAddress, to: UnsafePointer<Int8>.self)
                event.data_length = Int32(strlen(event.data))
                app_websocket_event(&event)
            }

        default:
            break
        }
    }
}
