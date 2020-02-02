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

load 'Session.rb'

username = 'testalpha'
password = 'same_as_my_luggage'
server = 'http://127.0.0.1:10999'

describe "API" do
    before(:all) do
        @serverProcess = fork do
            comp_manager = ENV['TESTING_DIR'] + '/bin/comp_manager'
            config_path = ENV['TESTING_DIR'] + '/bin/testing/programs-lobby.xml'

            Dir.chdir ENV['TESTING_DIR']
            Kernel.exec comp_manager, config_path
        end

        sleep 3.0
    end

    after(:all) do
        begin
            Process.kill "INT", @serverProcess

            # If the process does not exit, force it to.
            begin
                Timeout::timeout(3.0) {
                    Process.wait @serverProcess
                }
            rescue Timeout::Error
                Process.kill "KILL", @serverProcess
            end

            Process.wait @serverProcess
        rescue SystemCallError
            # Do nothing if an error occurs.
        end
    end

    describe "/account/get_challenge" do
        it "Bad user" do
            s = Session.new(server, 'hackerBob', 'hackMe')
            expect { s.Authenticate() }.to raise_error(Mechanize::ResponseCodeError)
        end

        it "Bad password" do
            s = Session.new(server, username, 'hackMe')
            s.Authenticate()
            expect { s.Request('/account/get_cp') }.to raise_error(Mechanize::ResponseCodeError)
        end

        it "No authentication" do
            s = Session.new(server, username, password)
            expect { s.Request('/account/get_cp') }.to raise_error(Mechanize::ResponseCodeError)
        end

        it "Bad challenge" do
            s = Session.new(server, username, password)
            s.Authenticate()
            s.Request('/account/get_cp')
            s.SetChallenge('123')
            expect { s.Request('/account/get_cp') }.to raise_error(Mechanize::ResponseCodeError)
        end

        it "Request with bad JSON" do
            s = Session.new(server, username, password)
            s.Authenticate()
            expect { s.RequestRaw('bad') }.to raise_error(Mechanize::ResponseCodeError)
        end

        it "Good authentication" do
            s = Session.new(server, username, password)
            s.Authenticate()
            s.Request('/account/get_cp')
        end
    end

    it "/bad/method" do
        s = Session.new(server, username, password)
        s.Authenticate()
        expect { s.Request('/bad/method') }.to raise_error(Mechanize::ResponseCodeError)
    end

    it "/account/get_cp" do
        s = Session.new(server, username, password)
        s.Authenticate()
        expect(s.Request('/account/get_cp')["cp"]).to eq(1000000)
        expect(s.Request('/account/get_cp')["cp"]).to eq(1000000)
    end
end
