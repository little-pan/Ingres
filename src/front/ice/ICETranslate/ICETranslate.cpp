/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: ICETranslate.cpp
**
** Description:
**    Main routine that controls the operation of the SAX parser to
**    translate the well formed XML based ICE language into the
**    ICE 2.0 language.
**

NEEDLIBS = ICETRANSLATELIB ICECONFLIB SHXERCESLIB
** History:
**      13-Mar-2001 (peeje01)
**          SIR 103810
**          Created.
**      13-Mar-2001 (peeje01)
**          SIR 103810
**          Correct program name in usage message
**	12-Mar-2004 (gupsh01)
**	    Updated to support Xerces 2.3. 
**	07-Jul-2004 (hanje04)
**	    Move main() so that mkjams/mkming does its job
**	29-Mar-2010 (drivi01)
**	    Clean up the warnings.
*/
/*
 * The Apache Software License, Version 1.1
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Xerces" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache\@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation, and was
 * originally based on software copyright (c) 1999, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Log: SAX2Print.cpp,v $
 * Revision 1.2  2000/08/09 22:20:38  jpolast
 * updates for changes to sax2 core functionality.
 *
 * Revision 1.1  2000/08/02 19:16:14  jpolast
 * initial checkin of SAX2Print
 *
 *
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/TransService.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
# include "ICETranslate.hpp"


// ---------------------------------------------------------------------------
//  Local data
//
//  encodingName
//      The encoding we are to output in. If not set on the command line,
//      then it is defaulted to LATIN1.
//
//  xmlFile
//      The path to the file to parser. Set via command line.
//
//  valScheme
//      Indicates what validation scheme to use. It defaults to 'auto', but
//      can be set via the -v= command.
//
//	expandNamespaces
//		Indicates if the output should expand the namespaces Alias with
//		their URI's, defaults to false, can be set via the command line -e
// ---------------------------------------------------------------------------
static const INGXMLCh*              encodingName    = (INGXMLCh *)"LATIN1";
static XMLFormatter::UnRepFlags unRepFlags      = XMLFormatter::UnRep_CharRef;
static char*                    xmlFile         = 0;
static SAX2XMLReader::ValSchemes valScheme      = SAX2XMLReader::Val_Auto;
static bool						expandNamespaces= false ;



// ---------------------------------------------------------------------------
//  Local helper methods
// ---------------------------------------------------------------------------
static void usage()
{
    cout <<  "\nUsage: ICETranslate [options] file\n"
             "This program prints the data returned by the various SAX2\n"
             "handlers for the specified input file. Options are NOT case\n"
             "sensitive.\n\n"
             "Options:\n"
             "    -u=xxx      Handle unrepresentable chars [fail | rep | ref*]\n"
             "    -v=xxx      Validation scheme [always | never | auto*]\n"
             "    -e          Expand Namespace Alias with URI's.\n"
             "    -x=XXX      Use a particular encoding for output (LATIN1*).\n"
             "    -?          Show this help\n\n"
             "  * = Default if not provided explicitly\n\n"
             "The parser has intrinsic support for the following encodings:\n"
             "    UTF-8, USASCII, ISO8859-1, UTF-16[BL]E, UCS-4[BL]E,\n"
             "    WINDOWS-1252, IBM1140, IBM037\n"
         <<  endl;
}



// ---------------------------------------------------------------------------
//  Program entry point
// ---------------------------------------------------------------------------
int
main(int argC, char* argV[])
{
 
	ios::sync_with_stdio();

	// Initialize the XML4C2 system
    try
    {
         XMLPlatformUtils::Initialize();
    }


    catch (const XMLException& toCatch)
    {
         cerr << "Error during initialization! :\n"
              << StrX(toCatch.getMessage()) << endl;
         return 1;
    }

    // Check command line and extract arguments.
    if (argC < 2)
    {
        usage();
        XMLPlatformUtils::Terminate();
        return 1;
    }

    // Watch for special case help request
    if ((argC == 2) && !strcmp(argV[1], "-?"))
    {
        usage();
        XMLPlatformUtils::Terminate();
        return 2;
    }

    int parmInd;
    for (parmInd = 1; parmInd < argC; parmInd++)
    {
        // Break out on first parm not starting with a dash
        if (argV[parmInd][0] != '-')
            break;

        if (!strncmp(argV[parmInd], "-v=", 3)
        ||  !strncmp(argV[parmInd], "-V=", 3))
        {
            const char* const parm = &argV[parmInd][3];

            if (!strcmp(parm, "never"))
                valScheme = SAX2XMLReader::Val_Never;
            else if (!strcmp(parm, "auto"))
                valScheme = SAX2XMLReader::Val_Auto;
            else if (!strcmp(parm, "always"))
                valScheme = SAX2XMLReader::Val_Always;
            else
            {
                cerr << "Unknown -v= value: " << parm << endl;
                return 2;
            }
        }
         else if (!strcmp(argV[parmInd], "-e")
              ||  !strcmp(argV[parmInd], "-E"))
        {
            expandNamespaces = true;
        }
         else if (!strncmp(argV[parmInd], "-x=", 3)
              ||  !strncmp(argV[parmInd], "-X=", 3))
        {
            // Get out the encoding name
            encodingName = (INGXMLCh *)&argV[parmInd][3];
        }
         else if (!strncmp(argV[parmInd], "-u=", 3)
              ||  !strncmp(argV[parmInd], "-U=", 3))
        {
            const char* const parm = &argV[parmInd][3];

            if (!strcmp(parm, "fail"))
                unRepFlags = XMLFormatter::UnRep_Fail;
            else if (!strcmp(parm, "rep"))
                unRepFlags = XMLFormatter::UnRep_Replace;
            else if (!strcmp(parm, "ref"))
                unRepFlags = XMLFormatter::UnRep_CharRef;
            else
            {
                cerr << "Unknown -u= value: " << parm << endl;
                return 2;
            }
        }
         else
        {
            cerr << "Unknown option '" << argV[parmInd]
                 << "', ignoring it\n" << endl;
        }
    }

    //
    //  And now we have to have only one parameter left and it must be
    //  the file name.
    //
    if (parmInd + 1 != argC)
    {
        usage();
        return 1;
    }
    xmlFile = argV[parmInd];

    //
    //  Create a SAX parser object. Then, according to what we were told on
    //  the command line, set it to validate or not.
    //
    SAX2XMLReader* parser = XMLReaderFactory::createXMLReader();
	if (valScheme == SAX2XMLReader::Val_Auto)
	{
	  parser->setFeature(XMLString::transcode("http://xml.org/sax/features/validation"), true);
	  parser->setFeature(XMLString::transcode("http://apache.org/xml/features/validation/dynamic"), true);
	}
    if (valScheme == SAX2XMLReader::Val_Never)
	{
	  parser->setFeature(XMLString::transcode("http://xml.org/sax/features/validation"), false);
	}
	if (valScheme == SAX2XMLReader::Val_Always)
	{
	  parser->setFeature(XMLString::transcode("http://xml.org/sax/features/validation"), true);
	  parser->setFeature(XMLString::transcode("http://apache.org/xml/features/validation/dynamic"), false);
	}

    //
    //  Create the handler object and install it as the document and error
    //  handler for the parser. Then parse the file and catch any exceptions
    //  that propogate out
    //
    try
    {
        SAX2ICETranslate handler(encodingName, unRepFlags, expandNamespaces);
        parser->setContentHandler(&handler);
        parser->setErrorHandler(&handler);
        parser->parse(xmlFile);
    }

    catch (const XMLException& toCatch)
    {
        cerr << "\nAn error occured\n  Error: "
             << StrX(toCatch.getMessage())
             << "\n" << endl;
        return -1;
    }

    // And call the termination method
    XMLPlatformUtils::Terminate();

    return 0;
}

