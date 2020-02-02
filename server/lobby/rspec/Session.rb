#!/usr/bin/env ruby
#
# This file is part of COMP_hack.
#
# Copyright (C) 2010-2020 COMP_hack Team <compomega@tutanota.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

require 'json'
require 'mechanize'

class Session
    def initialize(server, username, password)
        @server = server
        @username = username
        @password = password
    end

    def Authenticate()
        m = Mechanize.new
        response = JSON.parse(m.post(@server + '/api/auth/get_challenge', {'username' => @username}.to_json, {'Content-Type' => 'application/json'}).body())
        @salt = response['salt']
        @password_hash = Digest::SHA2.new(512).hexdigest(@password + @salt)
        @challenge = Digest::SHA2.new(512).hexdigest(@password_hash + response['challenge'])
    end

    def RequestRaw(api_method, request = '')
        m = Mechanize.new
        response = JSON.parse(m.post(@server + '/api' + api_method, request, {'Content-Type' => 'application/json'}).body())
        @challenge = Digest::SHA2.new(512).hexdigest(@password_hash + response['challenge'])

        return response
    end

    def Request(api_method, request = {})
        request['challenge'] = @challenge

        m = Mechanize.new
        response = JSON.parse(m.post(@server + '/api' + api_method, request.to_json, {'Content-Type' => 'application/json'}).body())
        @challenge = Digest::SHA2.new(512).hexdigest(@password_hash + response['challenge'])

        return response
    end

    def SetChallenge(challenge)
        @challenge = challenge
    end
end
